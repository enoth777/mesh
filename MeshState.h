#ifndef MESH_STATE_H
#define MESH_STATE_H

#include <Arduino.h>

constexpr int MESH_MAX_DEVICES = 7;

void setMeshDeviceIdentity(
  int device,
  const String& shortId
);

extern MeshDevice meshDevices[MESH_MAX_DEVICES];
extern int meshConnectedDevices;

void updateMeshDevice(int device, int x, int y);

#endif