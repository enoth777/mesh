#ifndef MESH_OUTPUT_H
#define MESH_OUTPUT_H

#include <Arduino.h>

constexpr int MESH_MAX_DAC = 4095;

int meshCoordinateToDac(
    int coordinate
);

void setMeshOutput(
    int channel,
    int coordinate
);

enum class MeshDisconnectMode {
  HOLD,
  ZERO
};

extern MeshDisconnectMode meshDisconnectMode;

#endif // MESH_OUTPUT_H