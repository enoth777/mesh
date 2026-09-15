#include <WiFi.h>
#include <WiFiManager.h>
#include <WebSocketsClient.h>

const char* RELAY_HOST = "mesh-relay.onrender.com";
const char* RELAY_PATH = "/ws?role=esp&room=mesh";

WebSocketsClient webSocket;

bool relayConnected = false;

unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 2000;

void forgetWifi() {
  Serial.println("Forgetting saved WiFi credentials...");

  WiFiManager wm;
  wm.resetSettings();

  WiFi.disconnect(true, true);
  delay(500);

  Serial.println("WiFi credentials erased.");
  Serial.println("Restarting...");

  delay(1000);
  ESP.restart();
}

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

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      relayConnected = true;
      Serial.println();
      Serial.println("MESH relay connected.");
      break;

    case WStype_DISCONNECTED:
      relayConnected = false;
      Serial.println();
      Serial.println("MESH relay disconnected.");
      break;

    case WStype_TEXT: {
      char message[64];

      size_t copyLength = min(length, sizeof(message) - 1);
      memcpy(message, payload, copyLength);
      message[copyLength] = '\0';

      int device;
      int x;
      int y;

      if (sscanf(message, "X,%d,%d,%d", &device, &x, &y) == 3) {
        if (
          device >= 1 && device <= 8 &&
          x >= 0 && x <= 126 &&
          y >= 0 && y <= 126
        ) {
          Serial.printf(
            "Device %d | X: %3d | Y: %3d\n",
            device,
            x,
            y
          );
        }
      }

      break;
    }

    default:
      break;
  }
}

// void connectWifi() {
//   WiFi.mode(WIFI_STA);

//   WiFiManager wm;

//   Serial.println("Connecting to WiFi...");

//   bool connected = wm.autoConnect(
//     "MESH-SETUP",
//     "12345678"
//   );

//   if (!connected) {
//     Serial.println("WiFi setup failed.");
//     delay(2000);
//     ESP.restart();
//   }

//   Serial.println();
//   Serial.println("Connected to existing WiFi.");

//   Serial.print("SSID: ");
//   Serial.println(WiFi.SSID());

//   Serial.print("IP: ");
//   Serial.println(WiFi.localIP());
// }

void startWifiSetup() {
  Serial.println();
  Serial.println("Starting WiFi setup...");

  WiFiManager wm;

  bool connected = wm.startConfigPortal(
    "MESH-SETUP",
    "12345678"
  );

  if (!connected) {
    Serial.println("WiFi setup failed.");
    return;
  }

  Serial.println("WiFi configured.");
  Serial.println("Restarting...");

  delay(1000);
  ESP.restart();
}

void connectWifi() {
  WiFi.mode(WIFI_STA);

  Serial.println("Connecting to saved WiFi...");

  WiFi.begin();

  unsigned long start = millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - start < 15000
  ) {
    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connection failed.");
    return;
  }

  Serial.println();
  Serial.println("Connected to existing WiFi.");

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
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
  // -------Serial Commands -------------
  if (Serial.available()) {
  String command = Serial.readStringUntil('\n');
  command.trim();

  if (command.equalsIgnoreCase("forget")) {
    forgetWifi();
  }

  if (command.equalsIgnoreCase("setup")) {
    startWifiSetup();
  }
  }


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