#include <Arduino.h>
#include <Wire.h>

#define BQ_ADDRESS 0x55
#define SDA_PIN 23
#define SCL_PIN 22

// Safety threshold: if either cell reads above this, we refuse to auto-clear
#define MAX_SAFE_CELL_MV 4300

bool sendAltManufacturerAccess(uint16_t subcommand) {
  Wire.beginTransmission(BQ_ADDRESS);
  Wire.write(0x3E);
  Wire.write(subcommand & 0xFF);
  Wire.write((subcommand >> 8) & 0xFF);
  return (Wire.endTransmission() == 0);
}

int readMacDataBlock(uint8_t *buffer, int maxLen) {
  Wire.beginTransmission(BQ_ADDRESS);
  Wire.write(0x40);
  if (Wire.endTransmission(false) != 0) return -1;
  Wire.requestFrom(BQ_ADDRESS, (uint8_t)maxLen);
  int i = 0;
  while (Wire.available() && i < maxLen) {
    buffer[i++] = Wire.read();
  }
  return i;
}

bool readPFStatus(uint32_t &pf) {
  if (!sendAltManufacturerAccess(0x0053)) return false;
  delay(50);
  uint8_t buf[32];
  int len = readMacDataBlock(buf, 32);
  if (len < 4) return false;
  pf = buf[0] | (buf[1] << 8) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
  return true;
}

bool readCellVoltages(uint16_t &cell1, uint16_t &cell2) {
  if (!sendAltManufacturerAccess(0x0071)) return false; // DAStatus1
  delay(50);
  uint8_t buf[32];
  int len = readMacDataBlock(buf, 32);
  if (len < 4) return false;
  cell1 = buf[0] | (buf[1] << 8); // CELL_VOLTAGE_1 offset 0
  cell2 = buf[2] | (buf[3] << 8); // CELL_VOLTAGE_2 offset 2
  return true;
}

bool sendUnsealKey() {
  Wire.beginTransmission(BQ_ADDRESS);
  Wire.write(0x00);
  Wire.write(0x14); Wire.write(0x04); // 0x0414 LSB, MSB
  if (Wire.endTransmission() != 0) return false;
  delay(100);

  Wire.beginTransmission(BQ_ADDRESS);
  Wire.write(0x00);
  Wire.write(0x72); Wire.write(0x36); // 0x3672 LSB, MSB
  return (Wire.endTransmission() == 0);
}

bool sendFullAccessKey(uint32_t key) {
  uint16_t lowWord = key & 0xFFFF;
  uint16_t highWord = (key >> 16) & 0xFFFF;

  Wire.beginTransmission(BQ_ADDRESS);
  Wire.write(0x00);
  Wire.write(lowWord & 0xFF);
  Wire.write((lowWord >> 8) & 0xFF);
  if (Wire.endTransmission() != 0) return false;
  delay(50);

  Wire.beginTransmission(BQ_ADDRESS);
  Wire.write(0x00);
  Wire.write(highWord & 0xFF);
  Wire.write((highWord >> 8) & 0xFF);
  return (Wire.endTransmission() == 0);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Serial.println("== Step 1: Read current cell voltages ==");
  uint16_t cell1, cell2;
  bool gotVoltages = readCellVoltages(cell1, cell2);

  if (!gotVoltages) {
    Serial.println("Failed to read cell voltages — aborting, will not attempt clear.");
    return;
  }

  Serial.print("Cell 1: "); Serial.print(cell1); Serial.println(" mV");
  Serial.print("Cell 2: "); Serial.print(cell2); Serial.println(" mV");

  if (cell1 > MAX_SAFE_CELL_MV || cell2 > MAX_SAFE_CELL_MV) {
    Serial.println();
    Serial.println("!! One or both cells exceed safe threshold. NOT clearing PF. !!");
    Serial.println("Investigate the cell/charger before proceeding.");
    return;
  }

  Serial.println("Cell voltages look normal. Proceeding to clear PF...");
  Serial.println();

  Serial.println("== Step 1b: Unseal (device may have re-sealed after prior reset) ==");
  sendUnsealKey();
  delay(200);

  Serial.println("== Step 2: Attempt FULL ACCESS ==");
  sendFullAccessKey(0xFFFFFFFF);
  delay(200);

  // Verify FULL ACCESS via OperationStatus bits 9:8, per driver notes
  // (checking register 0x00 directly for this is unreliable on this device)
  Serial.println("== Step 2b: Verify FULL ACCESS via OperationStatus ==");
  if (sendAltManufacturerAccess(0x0054)) {
    delay(50);
    uint8_t buf[32];
    int len = readMacDataBlock(buf, 32);
    if (len >= 4) {
      uint32_t opStatus = buf[0] | (buf[1] << 8) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
      uint8_t sec = (opStatus >> 8) & 0x03;
      Serial.print("SEC bits: 0b");
      Serial.println(sec, BIN);
      if (sec == 0b01) {
        Serial.println("-> Confirmed FULL ACCESS");
      } else {
        Serial.println("-> NOT in FULL ACCESS, clear will likely fail");
      }
    }
  }

  Serial.println();
  Serial.println("== Step 3: PermanentFailureDataReset (0x0029) ==");
  sendAltManufacturerAccess(0x0029);
  delay(500);

  Serial.println("== Step 3b: DeviceReset (0x0012) ==");
  sendAltManufacturerAccess(0x0012);
  delay(1000);

  Serial.println("== Step 4: Re-check PFStatus ==");
  uint32_t pf;
  if (readPFStatus(pf)) {
    Serial.print("Raw: 0x"); Serial.println(pf, HEX);
    Serial.println(pf == 0 ? "-> PF CLEARED" : "-> PF STILL LATCHED");
  }
}

void loop() {
  delay(5000);
}