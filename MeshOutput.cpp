#include <Arduino.h>

#include "MeshOutput.h"


extern MeshDisconnectMode meshDisconnectMode = MeshDisconnectMode::HOLD;

int meshCoordinateToDac(
    int coordinate
) {
    coordinate = constrain( // Constrain coordinate to valid range
        coordinate,
        0,
        126
    );

    return map( // Map coordinate to DAC value
        coordinate,
        0,
        126,
        0,
        MESH_MAX_DAC
    );
}


void setMeshOutput(
    int channel,
    int coordinate
) {
    int dacValue = meshCoordinateToDac(coordinate);


    //temporary workaround for DAC output on ESP32-C6
    Serial.print("OUT");
    Serial.print(channel);
    Serial.print("   X=");
    Serial.print(coordinate);
    Serial.print("   DAC=");
    Serial.println(dacValue);
}






