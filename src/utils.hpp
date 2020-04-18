#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <inttypes.h>

template <typename T>
uint32_t kmHToHz(T speed) {
  // 200 = 50hz
  auto test = static_cast<double>(speed);
  if(test > 240){
    test = 240;
  }
  double speedHzOutput = test * 2.5;
  ESP_LOGI("Speed", "%lf hz", speedHzOutput);
  return static_cast<uint32_t>(speedHzOutput);
}

template <typename T>
uint32_t RPMToHz(T rpm) {
  // 1000 = 30.9hz
  auto test = static_cast<double>(rpm);
  if(test > 8000){
    test = 8000;
  }
  double hz = test / 30;
  ESP_LOGI("RPM", "%lf hz", hz);
  return static_cast<uint32_t>(hz);
}


String readSet() {
  File file = SPIFFS.open("/config.json", FILE_READ);
  if (!file) {
    return String();
  }

  String str = file.readString();
  file.close();
  return str;
}

void removeSet() {
  bool isRemove = SPIFFS.remove("/config.json");
  if (isRemove) {
    ESP_LOGD("Setting", "Remove Success");
  } else {
    ESP_LOGE("Setting", "Remove Fail");
  }
}
void saveSet(String txt) {
  File file = SPIFFS.open("/config.json", FILE_WRITE);
  if (!file) {
    return;
  }
  file.print(txt);
  file.close();
}

struct sTelemetryData* parsePacket(uint8_t* packetBuffer, size_t length) {
  PacketBase packetHeader;
  
  if (length > 0) {
    memcpy(&packetHeader, packetBuffer, sizeof(PacketBase));
    switch (packetHeader.mPacketType) {
      case eCarPhysics: {
        auto data = new struct sTelemetryData;
        memcpy(data, packetBuffer, sizeof(struct sTelemetryData));
        ESP_LOGD("Debug", "input eCar");
        return data;
      }
      default:
        break;
    }
  }
  return nullptr;
}
#endif