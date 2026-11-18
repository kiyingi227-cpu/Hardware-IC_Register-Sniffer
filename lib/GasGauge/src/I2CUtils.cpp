#include "I2CUtils.h"

bool I2CUtils::sendAltManufacturerAccess(uint8_t address, uint16_t subcommand) {
    Wire.beginTransmission(address);
    Wire.write(0x3E);
    Wire.write(subcommand & 0xFF);
    Wire.write((subcommand >> 8) & 0xFF);
    return (Wire.endTransmission() == 0);
}

int I2CUtils::readMacDataBlock(uint8_t address, uint8_t *buffer, int maxLen) {
    Wire.beginTransmission(address);
    Wire.write(0x40);
    if (Wire.endTransmission(false) != 0) return -1;
    Wire.requestFrom(address, (uint8_t)maxLen);
    int i = 0;
    while (Wire.available() && i < maxLen) {
        buffer[i++] = Wire.read();
    }
    return i;
}

bool I2CUtils::readWord(uint8_t address, uint8_t command, uint16_t &result) {
    Wire.beginTransmission(address);
    Wire.write(command);
    if (Wire.endTransmission(false) != 0) return false;
    Wire.requestFrom(address, (uint8_t)2);
    if (Wire.available() < 2) return false;
    uint8_t lsb = Wire.read();
    uint8_t msb = Wire.read();
    result = (msb << 8) | lsb;
    return true;
}

bool I2CUtils::writeWord(uint8_t address, uint8_t command, uint16_t data) {
    Wire.beginTransmission(address);
    Wire.write(command);
    Wire.write(data & 0xFF);
    Wire.write((data >> 8) & 0xFF);
    return (Wire.endTransmission() == 0);
}

void I2CUtils::scanBus() {
    Serial.println("\n--- Scanning I2C Bus ---");
    uint8_t count = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("Found I2C device at 0x");
            if (addr < 16) Serial.print("0");
            Serial.println(addr, HEX);
            count++;
        }
    }
    if (count == 0) {
        Serial.println("No I2C devices found. Check wiring & pull-ups.");
    }
    Serial.println("------------------------\n");
}
