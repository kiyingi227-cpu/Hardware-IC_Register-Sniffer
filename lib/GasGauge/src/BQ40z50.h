#pragma once
#include "GasGaugeBase.h"
#include "I2CUtils.h"

class BQ40z50 : public GasGaugeBase {
public:
    explicit BQ40z50(uint8_t address = 0x16);

    const char* getName() const override { return "TI BQ40z50 (1S-4S)"; }
    uint8_t getAddress() const override { return _address; }
    uint8_t getCellCount() const override { return 4; }

    bool begin() override;
    bool readStatus(bool autoSealAfterRead = false) override;
    virtual bool fixDevice(uint16_t maxSafeCellMv = 4300) override;
    bool sealDevice() override;
    bool unsealDevice() override;
    bool sendFullAccessKey(uint32_t key = 0xFFFFFFFF);
    bool readCellVoltages(uint16_t *cellVoltages, uint8_t maxCells) override;
    bool readPFStatus(uint32_t &pf);
    bool readOperationStatus(uint32_t &opStatus);

private:
    uint8_t _address;
    void printBit(uint32_t val, int bit, const char *label);
};
