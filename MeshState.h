#ifndef MESH_STATE_H
#define MESH_STATE_H

#include <Arduino.h>

constexpr int MESH_MAX_DEVICES = 7;

struct MeshDevice {
    bool active = false;
    int x = 0;
    int y = 0;
    unsigned long lastUpdate = 0;
};

extern MeshDevice meshDevices[MESH_MAX_DEVICES];
extern int meshConnectedDevices;

void updateMeshDevice(int device, int x, int y);

#endif