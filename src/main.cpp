#include <Arduino.h>
#include <AsyncUDP.h>
#include <WiFi.h>

#include "./projectcars/udp2_5.hpp"
#include "./utils.hpp"

AsyncUDP udp;
const char* ssid = "***********";
const char* password = "***********";

unsigned int updateIndex(0);
unsigned int indexChange(0);
sParticipantsData pData;
sParticipantsData pData2;
sParticipantVehicleNamesData pVehicles;
sParticipantVehicleNamesData pVehicles2;
sVehicleClassNamesData pClasses;
sGameStateData stateData;
PacketBase packetHeader;

bool participantsReceived(false);
bool participantsReceived2(false);
bool stateReceived(false);
bool vehiclesReceived(false);
bool vehiclesReceived2(false);

void setup() {
  memset(&packetHeader, 0, sizeof(PacketBase));
  memset(&pData, 0, sizeof(sParticipantsData));
  memset(&pData2, 0, sizeof(sParticipantsData));
  memset(&stateData, 0, sizeof(sGameStateData));
  memset(&pVehicles, 0, sizeof(sParticipantVehicleNamesData));
  memset(&pVehicles2, 0, sizeof(sParticipantVehicleNamesData));
  memset(&pClasses, 0, sizeof(sVehicleClassNamesData));

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    ESP_LOGE("WiFi", "Connect Failed");
    ESP.restart();
  }
  if (udp.listen(SMS_UDP_PORT)) {
    ESP_LOGI("UDP", "UDP Listening on IP: %s", WiFi.localIP().toString().c_str());
    udp.onPacket([](AsyncUDPPacket packet) {
      ESP_LOGD("UDP", "UDP Packet Type: %s", packet.isBroadcast()
                       ? "Broadcast"
                       : packet.isMulticast() ? "Multicast" : "Unicast");
      ESP_LOGD("UDP", "From: %s:%u", packet.remoteIP().toString().c_str(),packet.remotePort());
      ESP_LOGD("UDP", "To : %s:%u", packet.localIP().toString().c_str(), packet.localPort());
      ESP_LOGD("UDP", "Len : %d", static_cast<int32_t>(packet.length()));
      parsePacket(packet.data(), packet.length());
    });
  }
}

void parsePacket(uint8_t* packetBuffer, size_t length) {
  if (length > 0) {
    memcpy(&packetHeader, packetBuffer, sizeof(PacketBase));
    switch (packetHeader.mPacketType) {
      case eParticipants:
        if (packetHeader.mPartialPacketIndex == 1) {
          memcpy(&pData, packetBuffer, sizeof(sParticipantsData));
        }
        if (packetHeader.mPartialPacketIndex == 2) {
          memcpy(&pData2, packetBuffer, sizeof(sParticipantsData));
          participantsReceived2 = true;
        }
        if (packetHeader.mPartialPacketIndex ==
            packetHeader.mPartialPacketNumber) {
          participantsReceived = true;
        }

        break;
      case eGameState:
        memcpy(&stateData, packetBuffer, sizeof(sGameStateData));
        stateReceived = true;
        break;
      case eParticipantVehicleNames: {
        // last packet are always the vehicle class names
        if (packetHeader.mPartialPacketIndex ==
            packetHeader.mPartialPacketNumber) {
          memcpy(&pClasses, packetBuffer, sizeof(sVehicleClassNamesData));
          vehiclesReceived = true;
        } else {
          if (packetHeader.mPartialPacketIndex == 1) {
            memcpy(&pVehicles, packetBuffer,
                   sizeof(sParticipantVehicleNamesData));
          }
          if (packetHeader.mPartialPacketIndex == 2) {
            memcpy(&pVehicles2, packetBuffer,
                   sizeof(sParticipantVehicleNamesData));
          }
        }
      }
      default:
        break;
    }
  }

  ESP_LOGI("Log",
      " Last packet size %d, Packet count %d, packet type count %d, group index %d, total in group %d - of type \n",
      length, packetHeader.mPacketNumber, packetHeader.mCategoryPacketNumber,
      packetHeader.mPartialPacketIndex, packetHeader.mPartialPacketNumber,
      packetHeader.mPacketVersion);
  if (stateReceived) {
    int gameState = stateData.mGameState & 7;
    int sessionState = stateData.mGameState >> 4;

    wprintf(L" Game State %i, gameState  %i, sessionState %i \n",
            stateData.mGameState, gameState, sessionState);
    wprintf(L" Race Participants  \n");
    if (participantsReceived) {
      for (int i(0); i < PARTICIPANTS_PER_PACKET; ++i) {
        if (pData.sName[i][0] != '\0') {
          wprintf(L" Name %S \n", pData.sName[i]);
        }
      }
      if (participantsReceived2) {
        for (int i(0); i < PARTICIPANTS_PER_PACKET; ++i) {
          if (pData2.sName[i][0] != '\0') {
            wprintf(L" Name %S \n", pData2.sName[i]);
          }
        }
      }
    }
    if (vehiclesReceived) {
      wprintf(L"Vehicle Names\n");
      for (int i(0); i < VEHICLES_PER_PACKET; ++i) {
        if (pVehicles.sVehicles[i].sName[0] != '\0') {
          wprintf(L"Vehicle Name %S, index %d, class %d \n",
                  pVehicles.sVehicles[i].sName[i],
                  pVehicles.sVehicles[i].sIndex, pVehicles.sVehicles[i].sClass);
        }
      }
      if (vehiclesReceived2) {
        for (int i(0); i < VEHICLES_PER_PACKET; ++i) {
          if (pVehicles2.sVehicles[i].sName[0] != '\0') {
            wprintf(L"Vehicle Name %S, index %d, class %d \n",
                    pVehicles2.sVehicles[i].sName[i],
                    pVehicles2.sVehicles[i].sIndex,
                    pVehicles2.sVehicles[i].sClass);
          }
        }
      }
      wprintf(L"Class Names\n");
      for (int i(0); i < CLASSES_SUPPORTED_PER_PACKET; ++i) {
        if (pClasses.sClasses[i].sName[0] != '\0') {
          wprintf(L"Class Name %S, index %d`\n", pClasses.sClasses[i].sName,
                  pClasses.sClasses[i].sClassIndex);
        }
      }
    }
  }
}
void loop() { 
  delay(1000); 
}
