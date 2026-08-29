# CircuitPython Test Firmware

Test firmware for the sensr-air board sensors, written for CircuitPython 9+.

## What it tests

| Sensor | Address | Driver | Coverage |
|--------|---------|--------|----------|
| SGP40 (VOC) | 0x59 | [adafruit_sgp40](https://github.com/adafruit/Adafruit_CircuitPython_SGP40) | raw signal + VOC index |
| STCC4 (CO2) | 0x64 | [adafruit_stcc4](https://github.com/adafruit/Adafruit_CircuitPython_STCC4) | self test + CO2 readings |
| SHT40 (temp/RH) | (via STCC4) | [adafruit_stcc4](https://github.com/adafruit/Adafruit_CircuitPython_STCC4) | temp/RH read through the STCC4 |
| BMV080 (PM) | 0x57 | none | I2C presence check only |

The SHT40 is not directly reachable on the I2C bus — it sits on the STCC4's
private second I2C bus, and the STCC4 reports its temperature and humidity
values as part of its measurement data.

**BMV080 limitation:** Bosch only distributes the BMV080 driver as
closed-source precompiled static libraries for specific MCU toolchains, so no
CircuitPython driver exists. This test only verifies the BMV080 answers on the
I2C bus at 0x57. Use the [Arduino](../arduino/) or [ESPHome](../esphome/)
tests for actual PM readings.

## Running on hardware

1. Install CircuitPython 9+ on a board with a STEMMA QT/Qwiic connector (or
   wire the sensr-air board's SDA/SCL/3V3/GND to your board's I2C pins).
   The code prefers `board.STEMMA_I2C()` and falls back to
   `board.SCL`/`board.SDA`.
2. Install the libraries onto the board with
   [circup](https://github.com/adafruit/circup):

   ```sh
   circup install adafruit_sgp40 adafruit_stcc4
   ```

3. Copy `code.py` to the `CIRCUITPY` drive.
4. Watch the serial console. You should see PASS lines for each sensor
   followed by continuous readings every 2 seconds.

If the STCC4's ADDR line is pulled high, change `STCC4_ADDR` to `0x65` at the
top of `code.py`.

## Build check

CircuitPython is interpreted, so the "build" is a compile check with
`mpy-cross` (this is what CI runs):

```sh
pip install mpy-cross
mpy-cross code.py -o /tmp/code.mpy
```

Or with docker compose:

```sh
docker compose run --rm build
```
