# Arduino Test Firmware

Test firmware for the sensr-air board sensors, targeting ESP32 boards
(default build target: ESP32-S3).

## What it tests

| Sensor | Address | Library | Coverage |
|--------|---------|---------|----------|
| SGP40 (VOC) | 0x59 | [Sensirion I2C SGP40](https://github.com/Sensirion/arduino-i2c-sgp40) | self test + raw VOC signal |
| STCC4 (CO2) | 0x64 | [Sensirion I2C STCC4](https://github.com/Sensirion/arduino-i2c-stcc4) | self test + CO2 readings |
| SHT40 (temp/RH) | (via STCC4) | [Sensirion I2C STCC4](https://github.com/Sensirion/arduino-i2c-stcc4) | temp/RH read through the STCC4 |
| BMV080 (PM) | 0x57 | [DFRobot_BMV080](https://github.com/DFRobot/DFRobot_BMV080) | PM1 / PM2.5 / PM10 readings |

The SHT40 is not directly reachable on the I2C bus — it sits on the STCC4's
private second I2C bus, and the STCC4 reports its temperature and humidity
values as part of its measurement data.

The BMV080 driver is Bosch's closed-source precompiled SDK, bundled in the
DFRobot library. It only supports Xtensa ESP32 targets (ESP32, ESP32-S2,
ESP32-S3), which is why this sketch targets ESP32.

## Wiring

Connect the sensr-air board to your ESP32 via the Qwiic/STEMMA QT connector,
or wire 3V3, GND, SDA, and SCL to the board's header. This is a 3.3 V board —
do not use 5 V.

The sketch uses the dev board's default `SDA`/`SCL` pins. To override, define
`I2C_SDA_PIN` / `I2C_SCL_PIN` at build time or edit the top of the sketch.

If the STCC4's ADDR line is pulled high, change `STCC4_ADDR` to `0x65`.

## Building with arduino-cli

```sh
arduino-cli config init --additional-urls \
    https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32

arduino-cli lib install "Sensirion Core" "Sensirion I2C SGP40" "Sensirion I2C STCC4"
arduino-cli config set library.enable_unsafe_install true
arduino-cli lib install --git-url https://github.com/DFRobot/DFRobot_BMV080.git

arduino-cli compile --fqbn esp32:esp32:esp32s3 sensor-test
arduino-cli upload --fqbn esp32:esp32:esp32s3 -p /dev/ttyACM0 sensor-test
```

## Building with docker compose

```sh
docker compose run --rm build
```

The compiled firmware is written to `./build/`. Flash it with `esptool` or
`arduino-cli upload`.

## Expected output

PASS/FAIL lines for each sensor on the serial console (115200 baud), then
continuous readings every 2 seconds:

```
[PASS] SGP40 self test result: 0xd400
[PASS] STCC4 self test
[PASS] BMV080 init id: ...

SGP40:  raw 30398
STCC4:  CO2 617 ppm | SHT40: 23.4 C, 41.2 %RH
BMV080: PM1 2.1, PM2.5 3.4, PM10 4.0 ug/m3
```
