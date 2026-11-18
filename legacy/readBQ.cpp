#include <Arduino.h>
#include <Wire.h>

#define BQ_ADDRESS 0x55
#define SDA_PIN 23
#define SCL_PIN 22

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

bool readWord(uint8_t command, uint16_t &result) {
  Wire.beginTransmission(BQ_ADDRESS);
  Wire.write(command);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(BQ_ADDRESS, (uint8_t)2);
  if (Wire.available() < 2) return false;
  uint8_t lsb = Wire.read();
  uint8_t msb = Wire.read();
  result = (msb << 8) | lsb;
  return true;
}

void printBit(uint32_t val, int bit, const char *label) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println((val >> bit) & 1);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Serial.println("== Step 1: Check OperationStatus (FET states) ==");
  if (sendAltManufacturerAccess(0x0054)) {
    delay(50);
    uint8_t buf[32];
    int len = readMacDataBlock(buf, 32);
    if (len >= 4) {
      uint32_t opStatus = buf[0] | (buf[1] << 8) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
      Serial.print("Raw: 0x"); Serial.println(opStatus, HEX);
      printBit(opStatus, 2, "CHG FET (1=on)");
      printBit(opStatus, 1, "DSG FET (1=on)");
      printBit(opStatus, 13, "XDSG (1=discharging disabled)");
      printBit(opStatus, 14, "XCHG (1=charging disabled)");
      printBit(opStatus, 12, "PF (1=permanent failure)");
    } else {
      Serial.println("Read failed");
    }
  }

  Serial.println();
  Serial.println("== Step 2: Read live current (sanity check for actual discharge) ==");
  uint16_t current;
  if (readWord(0x0C, current)) {
    Serial.print("Current: ");
    Serial.print((int16_t)current);
    Serial.println(" mA");
  }

  Serial.println();
  Serial.println("== Step 3: Re-seal device (SealDevice 0x0030) ==");
  if (sendAltManufacturerAccess(0x0030)) {
    Serial.println("Seal command sent");
  } else {
    Serial.println("Failed to send seal command");
  }

  delay(200);

  Serial.println();
  Serial.println("== Step 4: Confirm sealed state ==");
  uint16_t ctrl;
  if (readWord(0x00, ctrl)) {
    uint8_t sec = (ctrl >> 13) & 0x03;
    Serial.print("Control reg: 0x"); Serial.print(ctrl, HEX);
    Serial.print("  SEC bits: 0b"); Serial.println(sec, BIN);
     if (sec == 0b11) {
      Serial.println("-> Confirmed SEALED");
    } else {
      Serial.println("-> NOT sealed, unexpected");
    }
  }
}

void loop() {
  delay(5000);
}