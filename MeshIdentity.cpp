#include <Arduino.h>
#include <ESP.h>

#include "MeshIdentity.h"

// String getMeshHardwareId() {
//   uint64_t chipId = ESP.getEfuseMac();

//   uint32_t low32 =
//     static_cast<uint32_t>(chipId);

//   String hexId(low32, HEX);
//   hexId.toUpperCase();

//   while (hexId.length() < 8) {
//     hexId = "0" + hexId;
//   }

//   String hardwareId = "MESH-DEV-";
//   hardwareId += hexId;

//   return hardwareId;
// }



String getMeshHardwareId() {
  uint64_t chipId = ESP.getEfuseMac();

  uint32_t low32 =
    static_cast<uint32_t>(chipId);

  String hexId(low32, HEX);
  hexId.toUpperCase();

  while (hexId.length() < 8) {
    hexId = "0" + hexId;
  }

  String hardwareId = "MESH-DEV-";
  hardwareId += hexId;

  return hardwareId;
}