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

void setMeshOutputRaw(
    int channel,
    int dacValue
) {
    if (
        channel < 1 ||
        channel > MESH_MAX_OUTPUTS
    ) {
        Serial.printf(
            "Invalid channel: %d\n",
            channel
        );
        return;
    }

    dacValue = constrain( // Constrain DAC value to valid range
        dacValue,
        0,
        MESH_MAX_DAC
    );

    // Temporary fake DAC output  
    Serial.print("OUT");
    Serial.print(channel);
    Serial.print("   DAC=");
    Serial.println(dacValue);
}

void setMeshOutput(
    int channel,
    int coordinate
) {
    int dacValue = meshCoordinateToDac(coordinate);


    setMeshOutputRaw(
        channel,
        dacValue
    );

    // //temporary workaround for DAC output on ESP32-C6
    // Serial.print("OUT");
    // Serial.print(channel);
    // Serial.print("   X=");
    // Serial.print(coordinate);
    // Serial.print("   DAC=");
    // Serial.println(dacValue);
}






