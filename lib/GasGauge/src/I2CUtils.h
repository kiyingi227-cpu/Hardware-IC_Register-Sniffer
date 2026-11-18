#pragma once
#include <Arduino.h>
#include <Wire.h>

class I2CUtils {
public:
    static bool sendAltManufacturerAccess(uint8_t address, uint16_t subcommand);
    static int readMacDataBlock(uint8_t address, uint8_t *buffer, int maxLen);
    static bool readWord(uint8_t address, uint8_t command, uint16_t &result);
    static bool writeWord(uint8_t address, uint8_t command, uint16_t data);
    static void scanBus();
};
