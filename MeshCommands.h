#ifndef MESH_COMMANDS_H
#define MESH_COMMANDS_H

#include <Arduino.h>

void handleSerialCommands();

void executeMeshCommand(
  const String& command
);

void updateSerialMonitor();

#endif