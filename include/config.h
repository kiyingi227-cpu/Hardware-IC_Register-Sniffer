#pragma once
#include <Arduino.h>

// =============================================================================
// OPERATION MODE CONFIGURATION
// =============================================================================
enum OperationMode {
    MODE_READ,  // Telemetry, cell voltages, FET status, pack current, security status
    MODE_FIX    // Safety voltage validation, unseal, full access, clear PF (0x0029), device reset
};

// Set current mode: MODE_READ or MODE_FIX
constexpr OperationMode CURRENT_MODE = MODE_READ;

// In MODE_READ: whether to automatically re-seal the device after reading
constexpr bool AUTO_SEAL_AFTER_READ = true;

// =============================================================================
// TARGET IC CONFIGURATION
// =============================================================================
enum TargetIC {
    IC_BQ28Z610,  // TI BQ28z610 (1S-2S, Default I2C Addr: 0x55)
    IC_BQ40Z50   // TI BQ40z50  (1S-4S, Default I2C Addr: 0x16)
};

// Select which gas gauge IC you are communicating with:
constexpr TargetIC SELECTED_IC = IC_BQ28Z610;

// =============================================================================
// HARDWARE / I2C PIN CONFIGURATION
// =============================================================================
constexpr uint8_t SDA_PIN = 23;
constexpr uint8_t SCL_PIN = 22;
constexpr uint32_t I2C_CLOCK_SPEED = 100000; // 100 kHz standard mode

// =============================================================================
// SAFETY THRESHOLDS
// =============================================================================
// If any cell reads above this during MODE_FIX, clearing PF will be aborted.
constexpr uint16_t MAX_SAFE_CELL_MV = 4300;
