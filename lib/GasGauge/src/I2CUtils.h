#pragma once
#include <Arduino.h>
#include <Wire.h>

class I2CUtils {
public:
    // AltManufacturerAccess for BQ28z610 (subcommand to 0x3E, read from 0x40)
    static bool sendAltManufacturerAccess(uint8_t address, uint16_t subcommand);
    static int readMacDataBlock(uint8_t address, uint8_t *buffer, int maxLen);

    // ManufacturerBlockAccess for BQ40z50 (subcommand to 0x44, read from 0x44)
    static bool sendBlockSubcommand(uint8_t address, uint8_t cmdReg, uint16_t subcommand);
    static int readBlockData(uint8_t address, uint8_t cmdReg, uint8_t *buffer, int maxLen);

    // Standard SBS 16-bit word operations
    static bool readWord(uint8_t address, uint8_t command, uint16_t &result);
    static bool writeWord(uint8_t address, uint8_t command, uint16_t data);

    // Bus utility
    static void scanBus();
};
