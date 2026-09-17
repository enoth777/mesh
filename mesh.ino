#include <WiFi.h>
#include <WiFiManager.h>
#include <WebSocketsClient.h>
#include "MeshCommands.h"
#include "MeshState.h"
#include "MeshWifi.h"

const char* RELAY_HOST = "mesh-relay.onrender.com";
const char* RELAY_PATH = "/ws?role=esp&room=mesh";

WebSocketsClient webSocket;

bool relayConnected = false;

unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 2000;


void updateLed() {
  static unsigned long lastBlink = 0;
  static bool state = false;

  if (WiFi.status() == WL_CONNECTED && relayConnected) {
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }

  if (millis() - lastBlink >= 200) {
    lastBlink = millis();
    state = !state;
    digitalWrite(LED_BUILTIN, state ? LOW : HIGH);
  }
}

void webSocketEvent(
  WStype_t type,
  uint8_t* payload,
  size_t length
) {

  switch (type) {

    case WStype_CONNECTED: {
      relayConnected = true;

      Serial.println();
      Serial.println("MESH relay connected.");

      break;
    }

    case WStype_DISCONNECTED: {
      relayConnected = false;

      Serial.println();
      Serial.println("MESH relay disconnected.");

      break;
    }

    case WStype_TEXT: {

      char message[64];

      size_t copyLength =
        min(length, sizeof(message) - 1);

      memcpy(
        message,
        payload,
        copyLength
      );

      message[copyLength] = '\0';


      // Connected device count
      int connectedDevices;

      if (
        sscanf(
          message,
          "C,%d",
          &meshConnectedDevices
        ) == 1
      ) {
        if (
          meshConnectedDevices < 0 ||
          meshConnectedDevices > MESH_MAX_DEVICES
        ) {
          break;
        }

        for (
          int i = 0;
          i < MESH_MAX_DEVICES;
          i++
        ) {
          if (i >= meshConnectedDevices) {
            meshDevices[i].active = false;
          }
        }

        Serial.printf(
          "Connected devices: %d\n",
          meshConnectedDevices
        );

        break;
      }

      // Coordinates
      int device;
      int x;
      int y;

      int identityDevice;
      char identityShortId[5];

      if (
        sscanf(
          message,
          "I,%d,%4s",
          &identityDevice,
          identityShortId
        ) == 2
      ) {
        if (
          identityDevice >= 1 &&
          identityDevice <= MESH_MAX_DEVICES
        ) {
          setMeshDeviceIdentity(
            identityDevice,
            String(identityShortId)
          );

          Serial.printf(
            "Device %d identity: %s\n",
            identityDevice,
            identityShortId
          );
        }

        break;
      }


      if (
        sscanf(
          message,
          "X,%d,%d,%d",
          &device,
          &x,
          &y
        ) == 3
      ) {

        if (
          device >= 1 &&
          device <= MESH_MAX_DEVICES &&
          x >= 0 &&
          x <= 126 &&
          y >= 0 &&
          y <= 126
        ) {
          
          updateMeshDevice(
              device,
              x,
              y
          );

          MeshDevice& d =
            meshDevices[device - 1];

          // Serial.printf(
          //   "STATE CH%d | active=%d | X=%d | Y=%d | age=%lums\n",
          //   device,
          //   d.active,
          //   d.x,
          //   d.y,
          //   millis() - d.lastUpdate
          // );

          // Serial.printf(
          //   "Device %d | X: %3d | Y: %3d\n",
          //   device,
          //   x,
          //   y
          // );
        }
      }

      break;
    }

    case WStype_ERROR: {
      relayConnected = false;

      Serial.println(
        "MESH WebSocket ERROR."
      );

      break;
    }

    default: {
      break;
    }
  }
}


void setupRelay() {
  Serial.println("Connecting to MESH relay...");

  webSocket.beginSSL(
    RELAY_HOST,
    443,
    RELAY_PATH
  );

  webSocket.onEvent(webSocketEvent);

  webSocket.setReconnectInterval(1000);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println();
  Serial.println("======================");
  Serial.println("        MESH");
  Serial.println("======================");

  connectWifi();
  setupRelay();
}


void loop() {

  //------------- Handle Serial Commands ----------
  handleSerialCommands();

  //------------- Update Serial Monitor -----------
  updateSerialMonitor();


  //-------------WiFI STatus ---------------
  if (WiFi.status() != WL_CONNECTED) {
    relayConnected = false;

    WiFi.reconnect();

    delay(50);

    updateLed();

    return;
  }

  webSocket.loop();

  if (
    relayConnected &&
    millis() - lastHeartbeat >= HEARTBEAT_INTERVAL
  ) {
    lastHeartbeat = millis();

    webSocket.sendTXT("H");
  }

  

  updateLed();
}

