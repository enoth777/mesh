#include "MeshState.h"

MeshDevice meshDevices[MESH_MAX_DEVICES];

int meshConnectedDevices = 0;

void updateMeshDevice(
  int device,
  int x,
  int y
) {
  if (
    device < 1 ||
    device > MESH_MAX_DEVICES
  ) {
    return;
  }

  MeshDevice& d =
    meshDevices[device - 1];

  d.active = true;
  d.x = x;
  d.y = y;
  d.lastUpdate = millis();
}