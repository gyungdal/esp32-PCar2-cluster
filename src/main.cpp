#include <Arduino.h>
#include <AsyncUDP.h>
#include <WiFi.h>
#include <driver/ledc.h>

#include "./config.hpp"
#include "./projectcars/udp2_5.hpp"
#include "./utils.hpp"

const gpio_num_t SPEED_METER_PIN = GPIO_NUM_26;
const gpio_num_t RPM_METER_PIN = GPIO_NUM_27;
const ledc_channel_t SPEED_METER_PWM_CHANNEL = LEDC_CHANNEL_0;
const ledc_channel_t RPM_METER_PWM_CHANNEL = LEDC_CHANNEL_2;
const uint8_t PWM_RESOLUTION = 8;
const uint32_t PWM_DEFAULT_FREQ = 60;
struct sTelemetryData* buffer;

ledc_channel_config_t SPEED_METER_LEDC_CONFIG = {
    .gpio_num = SPEED_METER_PIN,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .channel = SPEED_METER_PWM_CHANNEL,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = PWM_DEFAULT_FREQ,
    .hpoint = 0,
};

ledc_channel_config_t RPM_METER_LEDC_CONFIG = {
    .gpio_num = RPM_METER_PIN,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .channel = RPM_METER_PWM_CHANNEL,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_1,
    .duty = PWM_DEFAULT_FREQ,
    .hpoint = 0,
};
AsyncUDP udp;

void wifiSetup() {
  WiFiManager wm;
  if (!wm.startConfigPortal()) {
    ESP.restart();
  }
  DynamicJsonDocument doc(JSON_DEFAULT_SIZE);
  doc["ssid"] = wm.getSSID();
  doc["password"] = wm.getPassword();
  String output;
  serializeJson(doc, output);
  ESP_LOGD("Setting Save", "%s", output.c_str());
  saveSet(output);
  ESP.restart();
}
void setup() {
  buffer = nullptr;
  ledc_timer_config_t ledc_timer;
  ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
  ledc_timer.duty_resolution = LEDC_TIMER_8_BIT;
  ledc_timer.freq_hz = PWM_DEFAULT_FREQ;
  ledc_timer.timer_num = LEDC_TIMER_0;
  ledc_timer_config(&ledc_timer);
  ledc_timer.timer_num = LEDC_TIMER_1;
  ledc_timer_config(&ledc_timer);
  ledc_channel_config(&SPEED_METER_LEDC_CONFIG);
  ledc_channel_config(&RPM_METER_LEDC_CONFIG);

  Serial.begin(115200);
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
      removeSet();
      ESP.restart();
  }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
  if (!SPIFFS.begin(true)) {
    ESP_LOGI("FS", "Failed to mount file system");
    ESP.restart();
  }
  String setting = readSet();
  ESP_LOGI("Setting", "%s", setting.c_str());
  while (setting.isEmpty()) {
    removeSet();
    ESP_LOGI("Setting", "Empty Setting");
    wifiSetup();
  }
  ESP_LOGI("Setting", "Exist Setting");
  DynamicJsonDocument doc(JSON_DEFAULT_SIZE);
  deserializeJson(doc, setting);
  WiFi.begin(doc["ssid"].as<String>().c_str(),
             doc["password"].as<String>().c_str());
  delay(3000);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(1000);
    removeSet();
    ESP.restart();
  }

  ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, 20);
  ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_1, RPMToHz<float>(1000));
  
  // ledcSetup(SPEED_METER_PWM_CHANNEL, PWM_DEFAULT_FREQ, PWM_RESOLUTION);
  // ledcSetup(RPM_METER_PWM_CHANNEL, PWM_DEFAULT_FREQ, PWM_RESOLUTION);
  //ledcAttachPin(SPEED_METER_PIN, SPEED_METER_PWM_CHANNEL);
  //ledcAttachPin(RPM_METER_PIN, RPM_METER_PWM_CHANNEL);
  //ledcSetup(SPEED_METER_PWM_CHANNEL, kmHToHz<float>(100), PWM_RESOLUTION);
  //ledcSetup(RPM_METER_PWM_CHANNEL, RPMToHz<unsigned short>(4000),
  //          PWM_RESOLUTION);
  //ledcWrite(SPEED_METER_PWM_CHANNEL, 128);
  //ledcWrite(RPM_METER_PWM_CHANNEL, 128);
  if (udp.listen(SMS_UDP_PORT)) {
    ESP_LOGI("UDP", "UDP Listening on IP: %s",
             WiFi.localIP().toString().c_str());
    udp.onPacket([](AsyncUDPPacket packet) {
      ESP_LOGD("UDP", "UDP Packet Type: %s",
               packet.isBroadcast()
                   ? "Broadcast"
                   : packet.isMulticast() ? "Multicast" : "Unicast");
      ESP_LOGD("UDP", "From: %s:%u", packet.remoteIP().toString().c_str(),
               packet.remotePort());
      ESP_LOGD("UDP", "To : %s:%u", packet.localIP().toString().c_str(),
               packet.localPort());
      ESP_LOGD("UDP", "Len : %d", static_cast<int32_t>(packet.length()));
      auto temp = parsePacket(packet.data(), packet.length());
      if (temp != nullptr) {
        if(buffer != nullptr){
          delete buffer;
        }
        buffer = temp;
      }
    });
  }
}

void loop() { 
  if(buffer != nullptr){
    if(kmHToHz<float>(buffer->sSpeed) > 1){
      ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, kmHToHz<float>(buffer->sSpeed));
    }
    if(RPMToHz<float>(buffer->sRpm) > 1){
      ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_1, RPMToHz<float>(buffer->sRpm));
    }
  }
}
