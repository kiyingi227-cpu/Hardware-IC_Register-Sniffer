#include "I2CUtils.h"

// BQ28z610 AltManufacturerAccess: send 16-bit subcommand to register 0x3E
bool I2CUtils::sendAltManufacturerAccess(uint8_t address, uint16_t subcommand) {
    Wire.beginTransmission(address);
    Wire.write(0x3E);
    Wire.write(subcommand & 0xFF);
    Wire.write((subcommand >> 8) & 0xFF);
    return (Wire.endTransmission() == 0);
}

// BQ28z610: read raw MAC data block from register 0x40
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

// BQ40z50: send 16-bit subcommand to register 0x44 (ManufacturerBlockAccess) or 0x00
bool I2CUtils::sendBlockSubcommand(uint8_t address, uint8_t cmdReg, uint16_t subcommand) {
    // SMBus Block Write: [cmdReg, byte_count=2, LSB, MSB]
    Wire.beginTransmission(address);
    Wire.write(cmdReg);
    Wire.write(0x02); // Length of 2 bytes
    Wire.write(subcommand & 0xFF);
    Wire.write((subcommand >> 8) & 0xFF);
    if (Wire.endTransmission() == 0) return true;

    // Fallback: standard word write [cmdReg, LSB, MSB]
    return writeWord(address, cmdReg, subcommand);
}

// BQ40z50: read block response from register 0x44 (ManufacturerBlockAccess)
int I2CUtils::readBlockData(uint8_t address, uint8_t cmdReg, uint8_t *buffer, int maxLen) {
    Wire.beginTransmission(address);
    Wire.write(cmdReg);
    if (Wire.endTransmission(false) != 0) return -1;

    // Request full payload (up to maxLen + 1 for length byte if SMBus)
    uint8_t requested = (maxLen + 1 > 36) ? 36 : (maxLen + 1);
    Wire.requestFrom(address, requested);
    if (!Wire.available()) return -1;

    // First byte in SMBus block read is the byte count
    uint8_t blockLen = Wire.read();
    int bytesToRead = (blockLen < maxLen) ? blockLen : maxLen;

    int i = 0;
    while (Wire.available() && i < bytesToRead) {
        buffer[i++] = Wire.read();
    }
    // Flush remaining
    while (Wire.available()) Wire.read();
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
            Serial.print(addr, HEX);
            if (addr == 0x55) Serial.print(" (Typical BQ28z610)");
            else if (addr == 0x16) Serial.print(" (Typical BQ40z50 / SBS Smart Battery)");
            Serial.println();
            count++;
        }
    }
    if (count == 0) {
        Serial.println("No I2C devices found. Check wiring & pull-ups.");
    }
    Serial.println("------------------------\n");
}
