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

bool BQ40z50::readOperationStatus(uint32_t &opStatus) {
    if (!I2CUtils::sendAltManufacturerAccess(_address, 0x0054)) return false;
    delay(50);
    uint8_t buf[32];
    int len = I2CUtils::readMacDataBlock(_address, buf, 32);
    if (len < 4) return false;
    opStatus = buf[0] | (buf[1] << 8) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
    return true;
}

bool BQ40z50::readPFStatus(uint32_t &pf) {
    if (!I2CUtils::sendAltManufacturerAccess(_address, 0x0053)) return false;
    delay(50);
    uint8_t buf[32];
    int len = I2CUtils::readMacDataBlock(_address, buf, 32);
    if (len < 4) return false;
    pf = buf[0] | (buf[1] << 8) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
    return true;
}

bool BQ40z50::readCellVoltages(uint16_t *cellVoltages, uint8_t maxCells) {
    if (!cellVoltages || maxCells < 4) return false;
    if (!I2CUtils::sendAltManufacturerAccess(_address, 0x0071)) return false; // DAStatus1
    delay(50);
    uint8_t buf[32];
    int len = I2CUtils::readMacDataBlock(_address, buf, 32);
    if (len < 8) return false;
    for (int i = 0; i < 4; i++) {
        cellVoltages[i] = buf[i * 2] | (buf[i * 2 + 1] << 8);
    }
    return true;
}

bool BQ40z50::unsealDevice() {
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
    return I2CUtils::sendAltManufacturerAccess(_address, 0x0030);
}

bool BQ40z50::readStatus(bool autoSealAfterRead) {
    Serial.println("==================================================");
    Serial.println("   [MODE: READ] BQ40z50 Diagnostic Status");
    Serial.println("==================================================");

    // 1. Cell Voltages
    uint16_t cells[4] = {0, 0, 0, 0};
    if (readCellVoltages(cells, 4)) {
        for (int i = 0; i < 4; i++) {
            Serial.print("Cell "); Serial.print(i + 1); Serial.print(": ");
            Serial.print(cells[i]); Serial.println(" mV");
        }
    } else {
        Serial.println("Failed to read cell voltages.");
    }

    // 2. Current
    uint16_t currentRaw;
    if (I2CUtils::readWord(_address, 0x0A, currentRaw)) { // SBS 0x0A Current
        Serial.print("Pack Current: ");
        Serial.print((int16_t)currentRaw);
        Serial.println(" mA");
    }

    // 3. OperationStatus & FETs
    uint32_t opStatus = 0;
    if (readOperationStatus(opStatus)) {
        Serial.print("OperationStatus: 0x"); Serial.println(opStatus, HEX);
        printBit(opStatus, 2, "CHG FET (1=on)");
        printBit(opStatus, 1, "DSG FET (1=on)");
        printBit(opStatus, 13, "XDSG (1=discharging disabled)");
        printBit(opStatus, 14, "XCHG (1=charging disabled)");
        printBit(opStatus, 12, "PF (1=permanent failure)");

        uint8_t sec = (opStatus >> 8) & 0x03;
        Serial.print("Security Bits: 0b"); Serial.print(sec, BIN);
        if (sec == 0b11) Serial.println(" (SEALED)");
        else if (sec == 0b10) Serial.println(" (UNSEALED)");
        else if (sec == 0b01) Serial.println(" (FULL ACCESS)");
        else Serial.println(" (RESERVED)");
    } else {
        Serial.println("Failed to read OperationStatus.");
    }

    // 4. Auto-seal
    if (autoSealAfterRead) {
        Serial.println("\nAuto-sealing device (0x0030)...");
        if (sealDevice()) {
            Serial.println("Seal command sent.");
        }
    }

    Serial.println("==================================================\n");
    return true;
}

bool BQ40z50::fixDevice(uint16_t maxSafeCellMv) {
    Serial.println("==================================================");
    Serial.println("   [MODE: FIX] BQ40z50 Permanent Failure Clear");
    Serial.println("==================================================");

    // Step 1: Read current cell voltages
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
            Serial.println("!! Cell exceeds safety threshold. ABORTING PF CLEAR !!");
            return false;
        }
    }
    Serial.println("Cell voltages within safe limits. Proceeding...");

    // Step 2: Unseal
    Serial.println("\n== Step 2: Unsealing device ==");
    unsealDevice();
    delay(200);

    // Step 3: Full Access
    Serial.println("== Step 3: Entering FULL ACCESS ==");
    sendFullAccessKey(0xFFFFFFFF);
    delay(200);

    // Step 4: Clear PF
    Serial.println("\n== Step 4: Sending PermanentFailureDataReset (0x0029) ==");
    I2CUtils::sendAltManufacturerAccess(_address, 0x0029);
    delay(500);

    // Step 5: Device Reset
    Serial.println("== Step 5: Sending DeviceReset (0x0012) ==");
    I2CUtils::sendAltManufacturerAccess(_address, 0x0012);
    delay(1000);

    // Step 6: Verify PF Status
    Serial.println("\n== Step 6: Verifying PFStatus ==");
    uint32_t pf = 0;
    if (readPFStatus(pf)) {
        Serial.print("PFStatus: 0x"); Serial.println(pf, HEX);
        if (pf == 0) {
            Serial.println("==> SUCCESS: PF CLEARED! <==");
            return true;
        } else {
            Serial.println("==> WARNING: PF STILL LATCHED. <==");
            return false;
        }
    } else {
        Serial.println("Failed to read PFStatus after reset.");
        return false;
    }
}
