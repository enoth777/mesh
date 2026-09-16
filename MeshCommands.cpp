#include <Arduino.h>
#include "MeshCommands.h"
#include "MeshState.h"

void handleSerialCommands() {

    if (!Serial.available()) {
        return;
    }

    String command =
        Serial.readStringUntil('\n');

    command.trim();
    
    if (command.length() == 0) {
        return;
    }

    if (
        command.equalsIgnoreCase("/help")
    ) {

        Serial.println();
        Serial.println("MESH COMMANDS");
        Serial.println("----------------------------");
        Serial.println("/help     Show available commands");
        return;
    }

    Serial.println("unrecognized command: " + command);
    Serial.println("Type /help for a list of commands.");
}