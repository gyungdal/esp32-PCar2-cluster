#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <inttypes.h>

template <typename T>
uint32_t kmHToHz(T speed) {
  // 200 = 50hz
  
  double speedHzOutput = static_cast<double>(speed) / 1.44;
  ESP_LOGI("Speed", "%lf hz", speedHzOutput);
  return static_cast<uint32_t>(speedHzOutput);
}

template <typename T>
uint32_t RPMToHz(T rpm) {
  // 1000 = 30.9hz
  double hz = static_cast<double>(rpm) / 30;
  ESP_LOGI("RPM", "%lf hz", hz);
  return static_cast<uint32_t>(hz);
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