#pragma once
#include <Arduino.h>

class GasGaugeBase {
public:
    virtual ~GasGaugeBase() = default;

    virtual const char* getName() const = 0;
    virtual uint8_t getAddress() const = 0;
    virtual uint8_t getCellCount() const = 0;

    virtual bool begin() = 0;
    virtual bool readStatus(bool autoSealAfterRead = false) = 0;
    virtual bool fixDevice(uint16_t maxSafeCellMv = 4300) = 0;
    virtual bool sealDevice() = 0;
    virtual bool unsealDevice() = 0;
    virtual bool readCellVoltages(uint16_t *cellVoltages, uint8_t maxCells) = 0;
};
