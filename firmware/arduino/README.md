# Arduino Test Firmware

Test firmware for the sensr-air board sensors, targeting ESP32 boards
(default build target: ESP32-S3).

## What it tests

| Sensor | Address | Library | Coverage |
|--------|---------|---------|----------|
| SGP41 (VOC/NOx) | 0x59 | [Sensirion I2C SGP41](https://github.com/Sensirion/arduino-i2c-sgp41) | self test + raw VOC and NOx signals |
| STCC4 (CO2) | 0x64 | [Sensirion I2C STCC4](https://github.com/Sensirion/arduino-i2c-stcc4) | self test + CO2 readings |
| SFA40 (HCHO) | 0x5D | [Sensirion I2C SFA4x](https://github.com/Sensirion/arduino-i2c-sfa4x) | serial number + formaldehyde, temp/RH |
| SHT40 (temp/RH) | (via STCC4) | [Sensirion I2C STCC4](https://github.com/Sensirion/arduino-i2c-stcc4) | temp/RH read through the STCC4 |
| BMV080 (PM) | 0x54 | [DFRobot_BMV080](https://github.com/DFRobot/DFRobot_BMV080) | PM1 / PM2.5 / PM10 readings |

The SHT40 is not directly reachable on the I2C bus — it sits on the STCC4's
private second I2C bus, and the STCC4 reports its temperature and humidity
values as part of its measurement data.

### SGP40 or SGP41?

Boards from `hw/air-stcc4/0.4.0` on carry an SGP41 (VOC **and** NOx); earlier
boards carry an SGP40 (VOC only). Both answer at 0x59 and both respond to the
same serial-number command, so the sketch tells them apart the only way you
can: by issuing the SGP4x "Get Feature Set" command (0x202F) and masking the
low 9 bits of the reply — `0x0020` is an SGP40, `0x0040` an SGP41. The sketch
prints which part it found and drives it with the matching Sensirion library,
so one build works on either board.

The SGP41 needs up to 10 seconds of NOx conditioning at 1 Hz after power-up;
`SRAW_NOx` reads 0 until that finishes, and the sketch marks those lines
`(conditioning)`.

The SFA40's self-test puts the sensor into a special mode for 5–6 minutes, so
the sketch doesn't run it — it reads the serial number and starts continuous
measurement instead.

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

arduino-cli lib install "Sensirion Core" "Sensirion I2C SGP40" \
    "Sensirion I2C SGP41" "Sensirion I2C SFA4x" "Sensirion I2C STCC4"
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
[PASS] SGP4x present (0x59) detected: SGP41
[PASS] STCC4 present (0x64)
[PASS] SFA40 present (0x5D)
[PASS] BMV080 present (0x54)

[PASS] SGP41 self test result: 0xd400
[PASS] STCC4 self test
[PASS] SFA40 init serial: ...
[PASS] BMV080 init id: ...

SGP41:  raw VOC 30398, raw NOx 15803
STCC4:  CO2 617 ppm | SHT40: 23.4 C, 41.2 %RH
SFA40:  HCHO 12.3 ppb | 23.6 C, 40.8 %RH (status 0x0000)
BMV080: PM1 2.1, PM2.5 3.4, PM10 4.0 ug/m3
```
