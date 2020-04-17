#include <Arduino.h>
#include <AsyncUDP.h>
#include <WiFi.h>

#include "./projectcars/udp2_5.hpp"
#include "./utils.hpp"

AsyncUDP udp;
const char* ssid = "***********";
const char* password = "***********";

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    ESP_LOGE("WiFi", "Connect Failed");
    while (1) {
      delay(1000);
    }
  }
  if (udp.listen(1234)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
    udp.onPacket([](AsyncUDPPacket packet) {
      Serial.print("UDP Packet Type: ");
      Serial.print(packet.isBroadcast()
                       ? "Broadcast"
                       : packet.isMulticast() ? "Multicast" : "Unicast");
      Serial.print(", From: ");
      Serial.print(packet.remoteIP());
      Serial.print(":");
      Serial.print(packet.remotePort());
      Serial.print(", To: ");
      Serial.print(packet.localIP());
      Serial.print(":");
      Serial.print(packet.localPort());
      Serial.print(", Length: ");
      Serial.print(packet.length());
      Serial.print(", Data: ");
      Serial.write(packet.data(), packet.length());
      Serial.println();
      // reply to the client
      packet.printf("Got %u bytes of data", packet.length());
    });
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}
int main() {
  unsigned short port(SMS_UDP_PORT);
  char packetBuffer[SMS_UDP_MAX_PACKETSIZE];
  PacketBase packetHeader;
  memset(&packetHeader, 0, sizeof(PacketBase));

  int iResult(0);
  WSADATA wsaData;
  SOCKET RecvSocket;

  sockaddr_in Receiver;
  Receiver.sin_family = AF_INET;
  Receiver.sin_port = htons(port);
  Receiver.sin_addr.s_addr = htonl(INADDR_ANY);

  sockaddr_in Sender;
  int SenderAddrSize(sizeof(Sender));

  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != NO_ERROR) {
    wprintf(L"WSAStartup failed with error %d\n", iResult);
    return 1;
  }

  RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (RecvSocket == INVALID_SOCKET) {
    wprintf(L"socket failed with error %d\n", WSAGetLastError());
    return 1;
  }

  iResult = bind(RecvSocket, (SOCKADDR*)&Receiver, sizeof(Receiver));
  if (iResult != 0) {
    wprintf(L" Failed to bind to socket with error %d\n", WSAGetLastError());
    return 1;
  }

  //------------------------------------------------------------------------------
  // TEST DISPLAY CODE
  //------------------------------------------------------------------------------
  unsigned int updateIndex(0);
  unsigned int indexChange(0);
  sParticipantsData pData;
  sParticipantsData pData2;
  sParticipantVehicleNamesData pVehicles;
  sParticipantVehicleNamesData pVehicles2;
  sVehicleClassNamesData pClasses;
  sGameStateData stateData;

  memset(&pData, 0, sizeof(sParticipantsData));
  memset(&pData2, 0, sizeof(sParticipantsData));
  memset(&stateData, 0, sizeof(sGameStateData));
  memset(&pVehicles, 0, sizeof(sParticipantVehicleNamesData));
  memset(&pVehicles2, 0, sizeof(sParticipantVehicleNamesData));
  memset(&pClasses, 0, sizeof(sVehicleClassNamesData));

  bool participantsReceived(false);
  bool participantsReceived2(false);
  bool stateReceived(false);
  bool vehiclesReceived(false);
  bool vehiclesReceived2(false);

  printf("ESC TO EXIT\n\n");
  while (true) {
    printf("Receiving Packets \n");
    iResult = recvfrom(RecvSocket, packetBuffer, SMS_UDP_MAX_PACKETSIZE, 0,
                       (SOCKADDR*)&Sender, &SenderAddrSize);
    if (iResult == SOCKET_ERROR) {
      wprintf(L"recvfrom failed with error %d\n", WSAGetLastError());
    } else {
      if (iResult > 0) {
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
    }
    wprintf(
        L" Last packet size %d, Packet count %d, packet type count %d, group "
        L"index %d, total in group %d - of type \n",
        iResult, packetHeader.mPacketNumber, packetHeader.mCategoryPacketNumber,
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
                    pVehicles.sVehicles[i].sIndex,
                    pVehicles.sVehicles[i].sClass);
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

    system("cls");

    if (_kbhit() && _getch() == 27)  // check for escape
    {
      break;
    }
  }
  //------------------------------------------------------------------------------

  closesocket(RecvSocket);

  return 0;
}
