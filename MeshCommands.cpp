#include <Arduino.h>
#include <Wifi.h>

#include "MeshCommands.h"
#include "MeshWifi.h"
#include "MeshState.h"
#include "MeshIdentity.h"
#include "MeshOutput.h"

bool stateMonitorActive = false;
unsigned long lastStateRefresh = 0;

unsigned long stateRefreshInterval = 250;




void printMeshInfo() {
  Serial.println();
  Serial.println("MESH INFO");
  Serial.println("----------------------------");

  Serial.println("Firmware:    0.1.0-dev");
  Serial.println("Hardware:    XIAO ESP32-C6");

  Serial.print("Unit ID:     ");
  Serial.println(getMeshHardwareId());

  Serial.print("Capacity:    ");
  Serial.println(MESH_MAX_DEVICES);

  Serial.print("WiFi:        ");
  Serial.println(
    WiFi.status() == WL_CONNECTED
      ? "connected"
      : "disconnected"
  );

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("SSID:        ");
    Serial.println(WiFi.SSID());

    Serial.print("IP:          ");
    Serial.println(WiFi.localIP());

    Serial.print("RSSI:        ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }

  Serial.print("Uptime:      ");
  Serial.print(millis() / 1000);
  Serial.println(" s");

  Serial.println();
}




void printMeshState() {

  Serial.print("\033[2J");  // Clear screen
  Serial.print("\033[H");   // Move cursor to top-left

  Serial.println();
  Serial.println("MESH STATE");
  Serial.println("----------------------------");

  // WiFi
  Serial.print("WiFi:       ");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("connected");

    Serial.print("SSID:       ");
    Serial.println(WiFi.SSID());

    Serial.print("RSSI:       ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }
  else {
    Serial.println("disconnected");

    Serial.println("SSID:       -");
    Serial.println("RSSI:       -");
  }

  // Uptime
  unsigned long uptime =
    millis() / 1000;

  Serial.print("Uptime:     ");
  Serial.print(uptime);
  Serial.println(" s");

  // Devices
  Serial.print("Devices:    ");
  Serial.print(meshConnectedDevices);
  Serial.print(" / ");
  Serial.println(MESH_MAX_DEVICES);

  Serial.println();
  Serial.println("CHANNELS");
  Serial.println("----------------------------");

  unsigned long now = millis();

  for (
    int i = 0;
    i < MESH_MAX_DEVICES;
    i++
  ) {

    MeshDevice& device =
      meshDevices[i];

    Serial.print("CH");
    Serial.print(i + 1);
    Serial.print("   ");

    if (!device.active) {
      Serial.println("EMPTY");
      continue;
    }

    Serial.print("ACTIVE   ");

    if (device.shortId.length() > 0) {
      Serial.print(device.shortId);
    }
    else {
      Serial.print("----");
    }

    Serial.print("   X=");
    Serial.print(device.x);

    Serial.print("   Y=");
    Serial.print(device.y);

    Serial.print("   age=");
    Serial.print(
      now - device.lastUpdate
    );

    Serial.println(" ms");
  }

  Serial.println();
}




void updateSerialMonitor() {

  if (!stateMonitorActive) {
    return;
  }

  unsigned long now = millis();

  if (
    now - lastStateRefresh <
    stateRefreshInterval
  ) {
    return;
  }

  lastStateRefresh = now;

  printMeshState();
}






  void handleSerialCommands() {

    // Nothing received through Serial
    if (!Serial.available()) {
      return;
    }

    // Read command
    String command =
      Serial.readStringUntil('\n');

    command.trim();

    // Ignore empty input
    if (command.length() == 0) {
      return;
    }

    executeMeshCommand(command);
  }

  void executeMeshCommand(const String& command) {


    // =========================
    // /info
    // =========================  
    if (
      command.equalsIgnoreCase("/info")
    ) {
      printMeshInfo();
      return;
    }

    // =========================
    // /help
    // =========================

    if (
      command.equalsIgnoreCase("/help")
    ) {
      Serial.println();
      Serial.println("MESH COMMANDS");
      Serial.println("----------------------------");
      Serial.println("/help ............................Show available commands");
      Serial.println("/info ............................Display MESH system information");
      Serial.println("/setup ...........................Start WiFi setup");
      Serial.println("/forget ..........................Forget saved WiFi");
      Serial.println("/state [ms] ......................Show live state monitor (default 250 ms, minimum 20 ms)");
      Serial.println("/exit ............................Stop live state monitor");
      Serial.println("/output disconnect [hold|zero] ...Show or set disconnect mode");

      return;
    } 


    // =========================
    // /setup
    // =========================

    if (command.equalsIgnoreCase("/setup")) { 
      startWifiSetup();

      return;
    }

    // =========================
    // /forget
    // =========================

    if (command.equalsIgnoreCase("/forget")) { 
      forgetWifi();

      return;
    }
  



    // =========================
    // /output disconnect
    // =========================
    if (command.equalsIgnoreCase("/output disconnect")) {
      Serial.print("Disconnect mode: ");
      
      if(meshDisconnectMode == MeshDisconnectMode::HOLD) {
        Serial.println("HOLD");
      }
      else {
        Serial.println("ZERO");
      }
    
      return;
    }

    if (command.equalsIgnoreCase("/output disconnect hold")) {
      meshDisconnectMode = MeshDisconnectMode::HOLD;
      Serial.println("Disconnect mode set to HOLD");
      return;
    }

    if (command.equalsIgnoreCase("/output disconnect zero")) {
      meshDisconnectMode = MeshDisconnectMode::ZERO;
      Serial.println("Disconnect mode set to ZERO");
      return;
    }


    // =========================
    // /state
    // =========================

    if (
      command.equalsIgnoreCase("/state") ||
      command.startsWith("/state ")
    ) {
      unsigned long requestedInterval = 250;

      if (command.length() > 6) {
        String argument =
          command.substring(7);

        argument.trim();

        long parsedInterval =
          argument.toInt();

        if (parsedInterval >= 20) {
          requestedInterval =
            parsedInterval;
        }
        else {
          Serial.println(
            "Invalid refresh rate. Minimum is 20 ms."
          );
          return;
        }
      }

      stateRefreshInterval =
        requestedInterval;

      stateMonitorActive = true;
      lastStateRefresh = 0;

      Serial.println();
      Serial.print(
        "Live state monitor started at "
      );
      Serial.print(stateRefreshInterval);
      Serial.println(" ms.");
      Serial.println(
        "Type /exit to stop."
      );

      return;
    }

   


    // =========================  
    // /exit
    // =========================
    if (
      command.equalsIgnoreCase("/exit")
    ) {
      stateMonitorActive = false;

      Serial.println();
      Serial.println("Live state monitor stopped.");

      return;
    }



    // =========================
    // Unknown command
    // =========================
    Serial.print("Unknown command: ");

    Serial.println(command);
    Serial.println("Type /help for available commands.");
  
  }
