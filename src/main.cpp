#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include <GasGauge.h>

GasGaugeBase *gauge = nullptr;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================================");
    Serial.println("   Hardware-IC Register Sniffer & Gas Gauge Tool");
    Serial.println("========================================================");

    // Initialize I2C Bus
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(I2C_CLOCK_SPEED);

    // Quick I2C scan to confirm connected devices
    I2CUtils::scanBus();

    // Instantiate selected gas gauge driver
    switch (SELECTED_IC) {
        case IC_BQ28Z610:
            gauge = new BQ28z610();
            break;
        case IC_BQ40Z50:
            gauge = new BQ40z50();
            break;
        default:
            Serial.println("Error: Unknown IC selection!");
            return;
    }

    Serial.print("Target Device: ");
    Serial.println(gauge->getName());
    Serial.print("Target I2C Addr: 0x");
    Serial.println(gauge->getAddress(), HEX);

    // Verify communication with target device
    if (!gauge->begin()) {
        Serial.println("\n[!] Error: Target IC did not acknowledge on I2C.");
        Serial.println("    Please check wiring (SDA/SCL), pull-up resistors, and power.");
        return;
    }

    Serial.println("[+] Target IC acknowledged successfully.\n");

    // Execute configured mode
    if (CURRENT_MODE == MODE_READ) {
        Serial.println(">> Running in MODE_READ <<\n");
        gauge->readStatus(AUTO_SEAL_AFTER_READ);
    } else if (CURRENT_MODE == MODE_FIX) {
        Serial.println(">> Running in MODE_FIX (Permanent Failure Clear) <<\n");
        gauge->fixDevice(MAX_SAFE_CELL_MV);
    }
}

void loop() {
    // Idle loop
    delay(5000);
}
