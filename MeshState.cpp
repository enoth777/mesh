#include "MeshState.h"

MeshDevice meshDevices[MESH_MAX_DEVICES];

int meshConnectedDevices = 0;

void setMeshDeviceIdentity(
  int device,
  const String& shortId
) {
  if (
    device < 1 ||
    device > MESH_MAX_DEVICES
  ) {
    return;
  }

MeshDevice& d =
  meshDevices[device - 1];

// A different participant has taken this channel.
// Do not let it inherit the previous participant's state.
if (
  d.shortId.length() > 0 &&
  d.shortId != shortId
) {
  d.x = 0;
  d.y = 0;
  d.lastUpdate = 0;
}
d.shortId = shortId;
d.active = true;

};

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