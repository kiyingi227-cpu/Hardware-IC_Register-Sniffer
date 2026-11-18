# Hardware-IC Register Sniffer & Gas Gauge Tool

Bare-metal firmware for embedded hardware validation, bus sniffing (I2C), and configuration/safety register management for battery fuel gauge and hardware ICs.

---

## Features
- **Modular Driver Architecture:** Object-oriented base driver (`GasGaugeBase`) with specific implementations for TI Impedance Track™ ICs (`BQ28z610`, `BQ40z50`), easily expandable to future ICs.
- **Selectable Operation Modes:** Switch between diagnostic telemetry reading and permanent failure recovery using a single configuration variable.
  - **`MODE_READ`:** Reads cell voltages, live pack current, FET states (`CHG`, `DSG`, `XCHG`, `XDSG`), permanent failure flags, and security states.
  - **`MODE_FIX`:** Performs cell voltage safety verification, automated unsealing, Full Access escalation, Permanent Failure (`0x0029`) clearing, and device reset (`0x0012`).
- **I2C Bus Auto-Discovery:** Scans and reports all connected I2C devices on startup.

---

## Configuration (`include/config.h`)

All project behaviors are configured in [include/config.h](file:///home/joseph/Documents/PlatformIO/Projects/reader/include/config.h):

```cpp
// 1. Choose Mode: MODE_READ or MODE_FIX
constexpr OperationMode CURRENT_MODE = MODE_READ;

// 2. Select Target IC: IC_BQ28Z610 or IC_BQ40Z50
constexpr TargetIC SELECTED_IC = IC_BQ28Z610;

// 3. Pin Configuration (ESP32 defaults)
constexpr uint8_t SDA_PIN = 23;
constexpr uint8_t SCL_PIN = 22;
```

---

## Supported ICs

| IC Model | Cell Range | Default I2C Address | Features |
| :--- | :--- | :--- | :--- |
| **BQ28z610** | 1S – 2S | `0x55` | 2-Cell Telemetry, FET status, PF Clear, Reset, Unseal |
| **BQ40z50** | 1S – 4S | `0x16` | 4-Cell Telemetry, FET status, PF Clear, Reset, Unseal |

---

## Wiring (ESP32 to Fuel Gauge)

| ESP32 Pin | Fuel Gauge Pin | Notes |
| :--- | :--- | :--- |
| **GPIO 23** | **SDA** | Requires external pull-up (e.g. 4.7kΩ to 3.3V) |
| **GPIO 22** | **SCL** | Requires external pull-up (e.g. 4.7kΩ to 3.3V) |
| **GND** | **VSS / GND** | Common ground connection |
