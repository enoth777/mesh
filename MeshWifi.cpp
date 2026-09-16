#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "MeshWifi.h"


void connectWifi() {
  WiFi.mode(WIFI_STA);

  Serial.println(
    "Connecting to saved WiFi..."
  );

  WiFi.begin();

  unsigned long start =
    millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - start < 15000
  ) {
    delay(250);
    Serial.print(".");
  }

  if (
    WiFi.status() != WL_CONNECTED
  ) {
    Serial.println();
    Serial.println(
      "WiFi connection failed."
    );

    return;
  }

  Serial.println();
  Serial.println(
    "Connected to existing WiFi."
  );

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}


void startWifiSetup() {
  Serial.println();
  Serial.println(
    "Starting WiFi setup..."
  );

  WiFiManager wm;

  bool connected =
    wm.startConfigPortal(
      "MESH-SETUP",
      "12345678"
    );

  if (!connected) {
    Serial.println(
      "WiFi setup failed."
    );

    return;
  }

  Serial.println(
    "WiFi configured."
  );

  Serial.println(
    "Restarting..."
  );

  delay(1000);

  ESP.restart();
}


void forgetWifi() {
  Serial.println(
    "Forgetting saved WiFi credentials..."
  );

  WiFiManager wm;

  wm.resetSettings();

  WiFi.disconnect(
    true,
    true
  );

  delay(500);

  Serial.println(
    "WiFi credentials erased."
  );

  Serial.println(
    "Restarting..."
  );

  delay(1000);

  ESP.restart();
}