#include <Arduino.h>
#include <AsyncUDP.h>
#include <WiFi.h>
#include <driver/ledc.h>

#include "./projectcars/udp2_5.hpp"
#include "./utils.hpp"


const gpio_num_t SPEED_METER_PIN = GPIO_NUM_26;
const gpio_num_t RPM_METER_PIN = GPIO_NUM_27;
const ledc_channel_t SPEED_METER_PWM_CHANNEL = LEDC_CHANNEL_0;
const ledc_channel_t RPM_METER_PWM_CHANNEL = LEDC_CHANNEL_2;
const uint8_t PWM_RESOLUTION = 8;
const double PWM_DEFAULT_FREQ = 10E8;

AsyncUDP udp;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    ESP_LOGE("WiFi", "Connect Failed");
    ESP.restart();
  }
  //ledcSetup(SPEED_METER_PWM_CHANNEL, PWM_DEFAULT_FREQ, PWM_RESOLUTION);
  //ledcSetup(RPM_METER_PWM_CHANNEL, PWM_DEFAULT_FREQ, PWM_RESOLUTION);
  ledcAttachPin(SPEED_METER_PIN, SPEED_METER_PWM_CHANNEL);
  ledcAttachPin(RPM_METER_PIN, RPM_METER_PWM_CHANNEL);
  ledcSetup(SPEED_METER_PWM_CHANNEL, kmHToHz<float>(100), PWM_RESOLUTION);
  ledcSetup(RPM_METER_PWM_CHANNEL, RPMToHz<unsigned short>(4000), PWM_RESOLUTION);
  ledcWrite(SPEED_METER_PWM_CHANNEL, 128);
  ledcWrite(RPM_METER_PWM_CHANNEL, 128);
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
        ledcSetup(SPEED_METER_PWM_CHANNEL, kmHToHz<float>(temp->sSpeed), PWM_RESOLUTION);
        ledcSetup(RPM_METER_PWM_CHANNEL, RPMToHz<unsigned short>(temp->sRpm), PWM_RESOLUTION);
        ledcWrite(SPEED_METER_PWM_CHANNEL, 128);
        ledcWrite(RPM_METER_PWM_CHANNEL, 128);
        delete temp;
      }
    });
  }
}

void loop() { delay(1000); }
