#include <Arduino.h>
#include <Wifi.h>

#include "MeshCommands.h"
#include "MeshWifi.h"
#include "MeshState.h"

bool stateMonitorActive = false;
unsigned long lastStateRefresh = 0;

constexpr unsigned long STATE_REFRESH_INTERVAL = 250;


void printMeshState() {

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

    Serial.print("X=");
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
    STATE_REFRESH_INTERVAL
  ) {
    return;
  }

  lastStateRefresh = now;

  
  Serial.print("\033[2J");
  Serial.print("\033[H");
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


  // =========================
  // /help
  // =========================

  if (
    command.equalsIgnoreCase("/help")
  ) {
    Serial.println();
    Serial.println("MESH COMMANDS");
    Serial.println("----------------------------");
    Serial.println("/help      Show available commands");
    Serial.println("/setup     Start WiFi setup");
    Serial.println("/forget    Forget saved WiFi");
    Serial.println("/state     Show live state monitor");
    Serial.println("/exit      Stop live state monitor");
    Serial.println();

    return;
  }


  // =========================
  // /setup
  // =========================

  if (
    command.equalsIgnoreCase("/setup")
  ) {
    startWifiSetup();

    return;
  }


  // =========================
  // /forget
  // =========================

  if (
    command.equalsIgnoreCase("/forget")
  ) {
    forgetWifi();

    return;
  }
  

  // =========================
  // /state 
  // =========================

   if (
        command.equalsIgnoreCase("/state")
        ) {
        stateMonitorActive = true;

        Serial.println();
        Serial.println("Live state monitor started.");
        Serial.println("Type /exit to stop.");

        // printMeshState();

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

  Serial.print(
    "Unknown command: "
  );

  Serial.println(command);

  Serial.println(
    "Type /help for available commands."
  );
}