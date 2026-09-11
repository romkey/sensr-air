// SPDX-License-Identifier: MIT
//
// Test firmware for the sensr-air board (Arduino / ESP32)
//
// Sensors under test (addresses as populated on hw/air-stcc4/0.4.0):
//   SGP41  (VOC/NOx, I2C 0x59) - Sensirion I2C SGP41 library; boards up to
//          0.3.0 carry an SGP40 instead, which is detected at runtime and
//          driven with the Sensirion I2C SGP40 library
//   STCC4  (CO2,     I2C 0x64) - Sensirion I2C STCC4 library
//   SFA40  (HCHO,    I2C 0x5D) - Sensirion I2C SFA4x library
//   SHT40  (temp/RH) - not directly on the bus; the STCC4 reads it over a
//          private second I2C bus and reports its values with each measurement
//   BMV080 (PM,      I2C 0x54) - DFRobot_BMV080 library (Bosch precompiled SDK;
//          ESP32/S2/S3 only)
//
// Target: ESP32-S3 (any ESP32 with Xtensa core works with the DFRobot library)
//
// Output: PASS/FAIL line per sensor on the serial console, then continuous
// readings every 2 seconds.

#include <Arduino.h>
#include <Wire.h>

#include <SensirionI2CSgp40.h>
#include <SensirionI2CSgp41.h>
#include <SensirionI2cSfa4x.h>
#include <SensirionI2cStcc4.h>
#include <DFRobot_BMV080.h>

// I2C pins; override for your dev board if needed
#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN SDA
#endif
#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN SCL
#endif

static const uint8_t SGP4X_ADDR = 0x59;
static const uint8_t STCC4_ADDR = STCC4_I2C_ADDR_64;  // 0x65 if ADDR pulled high
static const uint8_t SFA40_ADDR = SFA40_I2C_ADDR_5D;  // 0x5D
static const uint8_t BMV080_ADDR = 0x54;

// SGP4x "Get Feature Set" (0x202F). The low 9 bits of the reply identify the
// part: the SGP40 measures VOC only, the SGP41 measures VOC and NOx. Both sit
// at 0x59 and answer the same serial-number command, so the feature set is the
// only way to tell them apart.
static const uint16_t SGP4X_CMD_GET_FEATURESET = 0x202F;
static const uint16_t SGP40_FEATURESET = 0x0020;
static const uint16_t SGP41_FEATURESET = 0x0040;

static const uint32_t READ_INTERVAL_MS = 2000;
static const uint16_t SGP41_CONDITIONING_S = 10;  // must not exceed 10s

enum SgpVariant { SGP_UNKNOWN, SGP_40, SGP_41 };

SensirionI2CSgp40 sgp40;
SensirionI2CSgp41 sgp41;
SensirionI2cStcc4 stcc4;
SensirionI2cSfa4x sfa40;
DFRobot_BMV080_I2C bmv080(&Wire, BMV080_ADDR);

SgpVariant sgpVariant = SGP_UNKNOWN;
bool sgpOk = false;
bool stcc4Ok = false;
bool sfa40Ok = false;
bool bmv080Ok = false;
uint16_t conditioningLeft = SGP41_CONDITIONING_S;

static void report(const char* name, bool ok, const String& detail = "") {
  Serial.print(ok ? "[PASS] " : "[FAIL] ");
  Serial.print(name);
  if (detail.length()) {
    Serial.print(" ");
    Serial.print(detail);
  }
  Serial.println();
}

static bool i2cProbe(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

static void i2cScan() {
  Serial.print("I2C scan:");
  bool found = false;
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    if (i2cProbe(addr)) {
      Serial.printf(" 0x%02x", addr);
      found = true;
    }
  }
  if (!found) {
    Serial.print(" no devices found");
  }
  Serial.println();
}

// Sensirion's CRC-8 (polynomial 0x31, init 0xFF)
static uint8_t crc8(const uint8_t* data, size_t length) {
  uint8_t crc = 0xFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
    }
  }
  return crc;
}

static SgpVariant detectSgpVariant() {
  Wire.beginTransmission(SGP4X_ADDR);
  Wire.write(SGP4X_CMD_GET_FEATURESET >> 8);
  Wire.write(SGP4X_CMD_GET_FEATURESET & 0xFF);
  if (Wire.endTransmission() != 0) {
    return SGP_UNKNOWN;
  }
  delay(10);

  uint8_t reply[3] = {0};
  if (Wire.requestFrom(SGP4X_ADDR, (uint8_t)3) != 3) {
    return SGP_UNKNOWN;
  }
  for (uint8_t i = 0; i < 3; i++) {
    reply[i] = Wire.read();
  }
  if (crc8(reply, 2) != reply[2]) {
    return SGP_UNKNOWN;
  }

  uint16_t featureset = (((uint16_t)reply[0] << 8) | reply[1]) & 0x1FF;
  if (featureset == SGP41_FEATURESET) {
    return SGP_41;
  }
  if (featureset == SGP40_FEATURESET) {
    return SGP_40;
  }
  return SGP_UNKNOWN;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("sensr-air Arduino sensor test");
  Serial.println("==================================================");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  i2cScan();

  sgpVariant = detectSgpVariant();
  report("SGP4x present (0x59)", sgpVariant != SGP_UNKNOWN,
         String("detected: ") + (sgpVariant == SGP_41   ? "SGP41"
                                 : sgpVariant == SGP_40 ? "SGP40"
                                                        : "unknown"));
  report("STCC4 present (0x64)", i2cProbe(STCC4_ADDR));
  report("SFA40 present (0x5D)", i2cProbe(SFA40_ADDR));
  report("BMV080 present (0x54)", i2cProbe(BMV080_ADDR));
  Serial.println();

  int16_t error = 0;

  // SGP41 (or SGP40 on boards up to 0.3.0)
  if (sgpVariant == SGP_41) {
    sgp41.begin(Wire);
    uint16_t serialNumber[3];
    error = sgp41.getSerialNumber(serialNumber);
    if (error == 0) {
      uint16_t testResult = 0;
      error = sgp41.executeSelfTest(testResult);
      // 0xD400 = all tests passed (see SGP41 datasheet)
      sgpOk = (error == 0) && (testResult == 0xD400);
      report("SGP41 self test", sgpOk,
             String("result: 0x") + String(testResult, HEX));
    } else {
      report("SGP41 init", false, String("error: ") + error);
    }
  } else if (sgpVariant == SGP_40) {
    sgp40.begin(Wire);
    uint16_t serialNumber[3];
    uint8_t serialNumberSize = 3;
    error = sgp40.getSerialNumber(serialNumber, serialNumberSize);
    if (error == 0) {
      uint16_t testResult = 0;
      error = sgp40.executeSelfTest(testResult);
      // 0xD400 = all tests passed (see SGP40 datasheet)
      sgpOk = (error == 0) && (testResult == 0xD400);
      report("SGP40 self test", sgpOk,
             String("result: 0x") + String(testResult, HEX));
    } else {
      report("SGP40 init", false, String("error: ") + error);
    }
  }

  // STCC4 (+ SHT40 read through the STCC4)
  stcc4.begin(Wire, STCC4_ADDR);
  uint32_t productId = 0;
  uint64_t stcc4Serial = 0;
  error = stcc4.getProductId(productId, stcc4Serial);
  if (error == 0) {
    bool testResult = false;
    error = stcc4.checkSelfTest(testResult);
    stcc4Ok = (error == 0) && testResult;
    report("STCC4 self test", stcc4Ok);
    error = stcc4.startContinuousMeasurement();
    if (error != 0) {
      stcc4Ok = false;
      report("STCC4 start measurement", false, String("error: ") + error);
    }
  } else {
    report("STCC4 init", false, String("error: ") + error);
  }

  // SFA40. Its self-test puts the sensor into a special mode for 5-6 minutes,
  // so this test only reads the serial number and starts measuring.
  sfa40.begin(Wire, SFA40_ADDR);
  sfa40.stopContinuousMeasurement();  // the serial number needs the idle state
  delay(50);
  uint64_t sfa40Serial = 0;
  error = sfa40.getSerialNumber(sfa40Serial);
  if (error == 0) {
    error = sfa40.startContinuousMeasurement();
    sfa40Ok = (error == 0);
    report("SFA40 init", sfa40Ok,
           String("serial: ") + String((uint32_t)(sfa40Serial >> 32), HEX) +
               String((uint32_t)sfa40Serial, HEX));
  } else {
    report("SFA40 init", false, String("error: ") + error);
  }

  // BMV080
  if (bmv080.begin() == 0 && bmv080.openBmv080() == 0) {
    char id[13] = {0};
    bmv080.getBmv080ID(id);
    bmv080Ok = (bmv080.setBmv080Mode(CONTINUOUS_MODE) == 0);
    report("BMV080 init", bmv080Ok, String("id: ") + id);
  } else {
    report("BMV080 init", false);
  }

  Serial.println();
  if (sgpVariant == SGP_41 && sgpOk) {
    // The SGP41 needs up to 10s of NOx conditioning at 1Hz before its first
    // real measurement; SRAW NOx stays 0 until it finishes.
    Serial.printf("Conditioning the SGP41 (%us)...\n", SGP41_CONDITIONING_S);
  }

  Serial.println("Continuous readings (every 2s):");
  Serial.println("--------------------------------------------------");
  delay(2000);  // let the STCC4 and SFA40 finish their first measurements
}

void loop() {
  // default humidity/temperature compensation values per Sensirion docs
  const uint16_t defaultRh = 0x8000;
  const uint16_t defaultT = 0x6666;

  if (sgpOk && sgpVariant == SGP_41) {
    uint16_t srawVoc = 0;
    uint16_t srawNox = 0;
    int16_t error;
    if (conditioningLeft > 0) {
      error = sgp41.executeConditioning(defaultRh, defaultT, srawVoc);
      conditioningLeft--;
    } else {
      error = sgp41.measureRawSignals(defaultRh, defaultT, srawVoc, srawNox);
    }
    if (error == 0) {
      Serial.printf("SGP41:  raw VOC %u, raw NOx %u%s\n", srawVoc, srawNox,
                    conditioningLeft > 0 ? " (conditioning)" : "");
    } else {
      Serial.printf("SGP41:  read error %d\n", error);
    }
  } else if (sgpOk && sgpVariant == SGP_40) {
    uint16_t srawVoc = 0;
    int16_t error = sgp40.measureRawSignal(defaultRh, defaultT, srawVoc);
    if (error == 0) {
      Serial.printf("SGP40:  raw %u\n", srawVoc);
    } else {
      Serial.printf("SGP40:  read error %d\n", error);
    }
  }

  if (stcc4Ok) {
    int16_t co2 = 0;
    float temperature = 0.0f;
    float humidity = 0.0f;
    uint16_t status = 0;
    // temperature/humidity come from the SHT40, which the STCC4 reads over
    // its private I2C bus
    int16_t error = stcc4.readMeasurement(co2, temperature, humidity, status);
    if (error == 0) {
      Serial.printf("STCC4:  CO2 %d ppm | SHT40: %.1f C, %.1f %%RH\n", co2,
                    temperature, humidity);
    } else {
      Serial.printf("STCC4:  read error %d\n", error);
    }
  }

  if (sfa40Ok) {
    float hcho = 0.0f;
    float humidity = 0.0f;
    float temperature = 0.0f;
    uint16_t status = 0;
    // the SFA40 measures its own humidity/temperature and uses them to
    // compensate the formaldehyde signal
    int16_t error =
        sfa40.readMeasurementData(hcho, humidity, temperature, status);
    if (error == 0) {
      Serial.printf("SFA40:  HCHO %.1f ppb | %.1f C, %.1f %%RH (status 0x%04x)\n",
                    hcho, temperature, humidity, status);
    } else {
      Serial.printf("SFA40:  read error %d\n", error);
    }
  }

  if (bmv080Ok) {
    float pm1 = 0.0f, pm25 = 0.0f, pm10 = 0.0f;
    if (bmv080.getBmv080Data(&pm1, &pm25, &pm10)) {
      Serial.printf("BMV080: PM1 %.1f, PM2.5 %.1f, PM10 %.1f ug/m3\n", pm1,
                    pm25, pm10);
    } else {
      Serial.println("BMV080: no new data yet");
    }
  }

  Serial.println();
  delay(READ_INTERVAL_MS);
}
