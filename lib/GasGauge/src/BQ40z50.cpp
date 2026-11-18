#include "BQ40z50.h"

BQ40z50::BQ40z50(uint8_t address) : _address(address) {}

bool BQ40z50::begin() {
    Wire.beginTransmission(_address);
    return (Wire.endTransmission() == 0);
}

void BQ40z50::printBit(uint32_t val, int bit, const char *label) {
    Serial.print("  ");
    Serial.print(label);
    Serial.print(": ");
    Serial.println((val >> bit) & 1);
}

void BQ40z50::printPFFlags(uint32_t pf) {
    if (pf == 0) {
        Serial.println("  [+] No Permanent Failure flags set (Normal).");
        return;
    }
    Serial.println("  [!] Active PF Flags:");
    if (pf & (1 << 0))  Serial.println("      - SOV: Safety Cell Overvoltage");
    if (pf & (1 << 1))  Serial.println("      - SUV: Safety Cell Undervoltage");
    if (pf & (1 << 2))  Serial.println("      - SOCC: Safety Overcurrent in Charge");
    if (pf & (1 << 3))  Serial.println("      - SOCD: Safety Overcurrent in Discharge");
    if (pf & (1 << 4))  Serial.println("      - SOT: Safety Overtemperature");
    if (pf & (1 << 5))  Serial.println("      - SOTF: Safety Overtemperature FET");
    if (pf & (1 << 8))  Serial.println("      - CFETF: Charge FET Failure");
    if (pf & (1 << 9))  Serial.println("      - DFETF: Discharge FET Failure");
    if (pf & (1 << 12)) Serial.println("      - FUSE: Chemical Fuse Blown / Tripped");
    if (pf & (1 << 13)) Serial.println("      - AFE_P: AFE Permanent Failure");
    if (pf & (1 << 14)) Serial.println("      - AFE_C: AFE Communication Failure");
}

bool BQ40z50::readPackVoltage(uint16_t &voltageMv) {
    return I2CUtils::readWord(_address, 0x09, voltageMv);
}

bool BQ40z50::readPackCurrent(int16_t &currentMa) {
    uint16_t raw;
    if (!I2CUtils::readWord(_address, 0x0A, raw)) return false;
    currentMa = (int16_t)raw;
    return true;
}

bool BQ40z50::readTemperature(float &tempC) {
    uint16_t raw;
    if (!I2CUtils::readWord(_address, 0x08, raw)) return false;
    tempC = ((float)raw / 10.0f) - 273.15f; // Convert 0.1K to Celsius
    return true;
}

bool BQ40z50::readSOC(uint16_t &socPercent) {
    return I2CUtils::readWord(_address, 0x0D, socPercent);
}

bool BQ40z50::readCapacity(uint16_t &remainMah, uint16_t &fullMah) {
    bool ok = I2CUtils::readWord(_address, 0x0F, remainMah);
    ok &= I2CUtils::readWord(_address, 0x10, fullMah);
    return ok;
}

bool BQ40z50::readCellVoltages(uint16_t *cellVoltages, uint8_t maxCells) {
    if (!cellVoltages || maxCells < 4) return false;

    // SBS 1.1 Direct Register Read:
    // 0x3F = Cell 1, 0x3E = Cell 2, 0x3D = Cell 3, 0x3C = Cell 4
    bool ok = true;
    ok &= I2CUtils::readWord(_address, 0x3F, cellVoltages[0]);
    ok &= I2CUtils::readWord(_address, 0x3E, cellVoltages[1]);
    ok &= I2CUtils::readWord(_address, 0x3D, cellVoltages[2]);
    ok &= I2CUtils::readWord(_address, 0x3C, cellVoltages[3]);

    if (ok) return true;

    // Fallback: DAStatus1 (0x0071) via ManufacturerBlockAccess (0x44)
    if (!I2CUtils::sendBlockSubcommand(_address, 0x44, 0x0071)) return false;
    delay(50);
    uint8_t buf[36];
    int len = I2CUtils::readBlockData(_address, 0x44, buf, 36);
    if (len >= 8) {
        // If first 2 bytes echo the 0x0071 subcommand
        int offset = (len > 8 && buf[0] == 0x71 && buf[1] == 0x00) ? 2 : 0;
        for (int i = 0; i < 4; i++) {
            cellVoltages[i] = buf[offset + i * 2] | (buf[offset + i * 2 + 1] << 8);
        }
        return true;
    }
    return false;
}

bool BQ40z50::readOperationStatus(uint32_t &opStatus) {
    // Send 0x0054 to 0x44 (ManufacturerBlockAccess)
    if (!I2CUtils::sendBlockSubcommand(_address, 0x44, 0x0054)) return false;
    delay(50);
    uint8_t buf[36];
    int len = I2CUtils::readBlockData(_address, 0x44, buf, 36);
    if (len < 4) return false;

    int offset = (len > 4 && buf[0] == 0x54 && buf[1] == 0x00) ? 2 : 0;
    opStatus = buf[offset] | (buf[offset + 1] << 8) | 
               ((uint32_t)buf[offset + 2] << 16) | ((uint32_t)buf[offset + 3] << 24);
    return true;
}

bool BQ40z50::readPFStatus(uint32_t &pf) {
    // Send 0x0053 to 0x44
    if (!I2CUtils::sendBlockSubcommand(_address, 0x44, 0x0053)) return false;
    delay(50);
    uint8_t buf[36];
    int len = I2CUtils::readBlockData(_address, 0x44, buf, 36);
    if (len < 4) return false;

    int offset = (len > 4 && buf[0] == 0x53 && buf[1] == 0x00) ? 2 : 0;
    pf = buf[offset] | (buf[offset + 1] << 8) | 
         ((uint32_t)buf[offset + 2] << 16) | ((uint32_t)buf[offset + 3] << 24);
    return true;
}

bool BQ40z50::unsealDevice() {
    // Send unseal key 1 (0x0414) then key 2 (0x3672) to 0x00 (or 0x44)
    Wire.beginTransmission(_address);
    Wire.write(0x00);
    Wire.write(0x14); Wire.write(0x04);
    if (Wire.endTransmission() != 0) return false;
    delay(100);

    Wire.beginTransmission(_address);
    Wire.write(0x00);
    Wire.write(0x72); Wire.write(0x36);
    return (Wire.endTransmission() == 0);
}

bool BQ40z50::sendFullAccessKey(uint32_t key) {
    uint16_t lowWord = key & 0xFFFF;
    uint16_t highWord = (key >> 16) & 0xFFFF;

    Wire.beginTransmission(_address);
    Wire.write(0x00);
    Wire.write(lowWord & 0xFF);
    Wire.write((lowWord >> 8) & 0xFF);
    if (Wire.endTransmission() != 0) return false;
    delay(50);

    Wire.beginTransmission(_address);
    Wire.write(0x00);
    Wire.write(highWord & 0xFF);
    Wire.write((highWord >> 8) & 0xFF);
    return (Wire.endTransmission() == 0);
}

bool BQ40z50::sealDevice() {
    return I2CUtils::sendBlockSubcommand(_address, 0x44, 0x0030);
}

bool BQ40z50::readStatus(bool autoSealAfterRead) {
    Serial.println("==================================================");
    Serial.println("   [MODE: READ] BQ40z50 Diagnostic Status");
    Serial.println("==================================================");

    // 1. Pack Telemetry (Standard SBS)
    uint16_t packVolt = 0;
    int16_t packCurr = 0;
    float tempC = 0.0f;
    uint16_t soc = 0;
    uint16_t remCap = 0, fullCap = 0;

    if (readPackVoltage(packVolt)) {
        Serial.print("Pack Voltage:    "); Serial.print(packVolt); Serial.println(" mV");
    }
    if (readPackCurrent(packCurr)) {
        Serial.print("Pack Current:    "); Serial.print(packCurr); Serial.println(" mA");
    }
    if (readTemperature(tempC)) {
        Serial.print("Temperature:     "); Serial.print(tempC, 1); Serial.println(" °C");
    }
    if (readSOC(soc)) {
        Serial.print("State of Charge: "); Serial.print(soc); Serial.println(" %");
    }
    if (readCapacity(remCap, fullCap)) {
        Serial.print("Capacity:        "); Serial.print(remCap); 
        Serial.print(" / "); Serial.print(fullCap); Serial.println(" mAh");
    }

    // 2. Individual Cell Voltages
    Serial.println("\nCell Voltages:");
    uint16_t cells[4] = {0, 0, 0, 0};
    if (readCellVoltages(cells, 4)) {
        for (int i = 0; i < 4; i++) {
            Serial.print("  Cell "); Serial.print(i + 1); Serial.print(": ");
            Serial.print(cells[i]); Serial.println(" mV");
        }
    } else {
        Serial.println("  Failed to read cell voltages.");
    }

    // 3. OperationStatus & FETs
    Serial.println("\nOperationStatus (FETs & Safety):");
    uint32_t opStatus = 0;
    if (readOperationStatus(opStatus)) {
        Serial.print("  Raw: 0x"); Serial.println(opStatus, HEX);
        printBit(opStatus, 2, "CHG FET (1=on)");
        printBit(opStatus, 1, "DSG FET (1=on)");
        printBit(opStatus, 13, "XDSG (1=discharge inhibited)");
        printBit(opStatus, 14, "XCHG (1=charge inhibited)");
        printBit(opStatus, 12, "PF (1=permanent failure latched)");

        uint8_t sec = (opStatus >> 8) & 0x03;
        Serial.print("  Security Status: 0b"); Serial.print(sec, BIN);
        if (sec == 0b11) Serial.println(" (SEALED)");
        else if (sec == 0b10) Serial.println(" (UNSEALED)");
        else if (sec == 0b01) Serial.println(" (FULL ACCESS)");
        else Serial.println(" (RESERVED)");
    } else {
        Serial.println("  Failed to read OperationStatus via MAC.");
    }

    // 4. Permanent Failure Status
    Serial.println("\nPermanent Failure Status (0x0053):");
    uint32_t pf = 0;
    if (readPFStatus(pf)) {
        Serial.print("  Raw PFStatus: 0x"); Serial.println(pf, HEX);
        printPFFlags(pf);
    } else {
        Serial.println("  Failed to read PFStatus.");
    }

    // 5. Auto-seal if requested
    if (autoSealAfterRead) {
        Serial.println("\nAuto-sealing device (0x0030)...");
        if (sealDevice()) {
            Serial.println("Seal command sent successfully.");
        } else {
            Serial.println("Failed to send seal command.");
        }
    }

    Serial.println("==================================================\n");
    return true;
}

bool BQ40z50::fixDevice(uint16_t maxSafeCellMv) {
    Serial.println("==================================================");
    Serial.println("   [MODE: FIX] BQ40z50 Permanent Failure Clear");
    Serial.println("==================================================");

    // Step 1: Read all cell voltages and ensure safe limits
    Serial.println("== Step 1: Reading cell voltages ==");
    uint16_t cells[4] = {0, 0, 0, 0};
    if (!readCellVoltages(cells, 4)) {
        Serial.println("Failed to read cell voltages — aborting fix!");
        return false;
    }

    for (int i = 0; i < 4; i++) {
        Serial.print("Cell "); Serial.print(i + 1); Serial.print(": ");
        Serial.print(cells[i]); Serial.println(" mV");
        if (cells[i] > maxSafeCellMv) {
            Serial.println();
            Serial.print("!! Cell "); Serial.print(i + 1);
            Serial.println(" exceeds safe voltage threshold! ABORTING PF CLEAR !!");
            Serial.println("Inspect cell/hardware before clearing PF.");
            return false;
        }
    }
    Serial.println("Cell voltages within safe limits. Proceeding...");

    // Step 2: Unseal
    Serial.println("\n== Step 2: Unsealing device (0x0414, 0x3672) ==");
    unsealDevice();
    delay(200);

    // Step 3: Full Access
    Serial.println("== Step 3: Entering FULL ACCESS ==");
    sendFullAccessKey(0xFFFFFFFF);
    delay(200);

    // Verify Full Access
    uint32_t opStatus = 0;
    if (readOperationStatus(opStatus)) {
        uint8_t sec = (opStatus >> 8) & 0x03;
        Serial.print("Security bits: 0b"); Serial.println(sec, BIN);
        if (sec == 0b01) {
            Serial.println("-> Confirmed FULL ACCESS");
        } else {
            Serial.println("-> WARNING: Not in FULL ACCESS. Key may be custom.");
        }
    }

    // Step 4: Clear Permanent Failure
    Serial.println("\n== Step 4: Sending PermanentFailureDataReset (0x0029) ==");
    I2CUtils::sendBlockSubcommand(_address, 0x44, 0x0029);
    delay(500);

    // Step 5: Device Reset
    Serial.println("== Step 5: Sending DeviceReset (0x0012) ==");
    I2CUtils::sendBlockSubcommand(_address, 0x44, 0x0012);
    delay(1000);

    // Step 6: Verify PF Status
    Serial.println("\n== Step 6: Verifying PFStatus after reset ==");
    uint32_t pf = 0;
    if (readPFStatus(pf)) {
        Serial.print("PFStatus: 0x"); Serial.println(pf, HEX);
        printPFFlags(pf);
        if (pf == 0) {
            Serial.println("\n==> SUCCESS: BQ40z50 PF CLEARED! FETs unlocked. <==");
            return true;
        } else {
            Serial.println("\n==> WARNING: PF STILL LATCHED. Cause must be resolved first. <==");
            return false;
        }
    } else {
        Serial.println("Failed to read PFStatus after reset.");
        return false;
    }
}
