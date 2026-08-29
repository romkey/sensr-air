// SPDX-License-Identifier: MIT
//
// Test firmware for the sensr-air board (Arduino / ESP32)
//
// Sensors under test:
//   SGP40  (VOC,     I2C 0x59) - Sensirion I2C SGP40 library
//   STCC4  (CO2,     I2C 0x64) - Sensirion I2C STCC4 library
//   SHT40  (temp/RH) - not directly on the bus; the STCC4 reads it over a
//          private second I2C bus and reports its values with each measurement
//   BMV080 (PM,      I2C 0x57) - DFRobot_BMV080 library (Bosch precompiled SDK;
//          ESP32/S2/S3 only)
//
// Target: ESP32-S3 (any ESP32 with Xtensa core works with the DFRobot library)
//
// Output: PASS/FAIL line per sensor on the serial console, then continuous
// readings every 2 seconds.

#include <Arduino.h>
#include <Wire.h>

#include <SensirionI2CSgp40.h>
#include <SensirionI2cStcc4.h>
#include <DFRobot_BMV080.h>

// I2C pins; override for your dev board if needed
#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN SDA
#endif
#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN SCL
#endif

static const uint8_t SGP40_ADDR = 0x59;
static const uint8_t STCC4_ADDR = STCC4_I2C_ADDR_64;  // 0x65 if ADDR pulled high
static const uint8_t BMV080_ADDR = DFRobot_BMV080_I2C_ADDR;  // 0x57

static const uint32_t READ_INTERVAL_MS = 2000;

SensirionI2CSgp40 sgp40;
SensirionI2cStcc4 stcc4;
DFRobot_BMV080_I2C bmv080(&Wire, BMV080_ADDR);

bool sgp40Ok = false;
bool stcc4Ok = false;
bool bmv080Ok = false;

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

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("sensr-air Arduino sensor test");
  Serial.println("==================================================");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  i2cScan();
  report("SGP40 present (0x59)", i2cProbe(SGP40_ADDR));
  report("STCC4 present (0x64)", i2cProbe(STCC4_ADDR));
  report("BMV080 present (0x57)", i2cProbe(BMV080_ADDR));
  Serial.println();

  // SGP40
  sgp40.begin(Wire);
  uint16_t serialNumber[3];
  uint8_t serialNumberSize = 3;
  int16_t error = sgp40.getSerialNumber(serialNumber, serialNumberSize);
  if (error == 0) {
    uint16_t testResult = 0;
    error = sgp40.executeSelfTest(testResult);
    // 0xD400 = all tests passed (see SGP40 datasheet)
    sgp40Ok = (error == 0) && (testResult == 0xD400);
    report("SGP40 self test", sgp40Ok,
           String("result: 0x") + String(testResult, HEX));
  } else {
    report("SGP40 init", false, String("error: ") + error);
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
  Serial.println("Continuous readings (every 2s):");
  Serial.println("--------------------------------------------------");
  delay(2000);  // let the STCC4 finish its first measurement
}

void loop() {
  if (sgp40Ok) {
    // default humidity/temperature compensation values per Sensirion docs
    uint16_t srawVoc = 0;
    int16_t error = sgp40.measureRawSignal(0x8000, 0x6666, srawVoc);
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
