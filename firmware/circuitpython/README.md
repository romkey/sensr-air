# CircuitPython Test Firmware

Test firmware for the sensr-air board sensors, written for CircuitPython 9+.

## What it tests

| Sensor | Address | Driver | Coverage |
|--------|---------|--------|----------|
| SGP41 (VOC/NOx) | 0x59 | [adafruit_sgp41](https://github.com/adafruit/Adafruit_CircuitPython_SGP41) | self test + raw signals + VOC/NOx index |
| STCC4 (CO2) | 0x64 | [adafruit_stcc4](https://github.com/adafruit/Adafruit_CircuitPython_STCC4) | self test + CO2 readings |
| SFA40 (HCHO) | 0x5D | [sfa40](https://github.com/romkey/circuitpython-sfa40) | serial number + formaldehyde, temp/RH |
| SHT40 (temp/RH) | (via STCC4) | [adafruit_stcc4](https://github.com/adafruit/Adafruit_CircuitPython_STCC4) | temp/RH read through the STCC4 |
| BMV080 (PM) | 0x54 | none | I2C presence check only |

The SHT40 is not directly reachable on the I2C bus — it sits on the STCC4's
private second I2C bus, and the STCC4 reports its temperature and humidity
values as part of its measurement data.

### SGP40 or SGP41?

Boards from `hw/air-stcc4/0.4.0` on carry an SGP41 (VOC **and** NOx); earlier
boards carry an SGP40 (VOC only). Both answer at 0x59 and both respond to the
same serial-number command, so the code tells them apart the only way you can:
by reading the SGP4x "Get Feature Set" command (0x202F) and masking the low 9
bits of the reply — `0x0020` is an SGP40, `0x0040` an SGP41. This is exactly
what ESPHome's `sgp4x` component does. The test prints which part it found and
loads the matching driver, so the same `code.py` works on either board.

The SGP41 needs up to 10 seconds of NOx conditioning at 1 Hz after power-up
before its first real measurement, which the test runs before it starts
printing readings. The gas index algorithm then warms up for about 45 seconds,
returning 0 for both indices until it settles.

**SFA40 self test:** the SFA40's self-test puts the sensor into a special mode
for 5–6 minutes, so this test doesn't run it. Use the driver's
`sfa40_self_test.py` example if you want to run it after assembly.

**BMV080 limitation:** Bosch only distributes the BMV080 driver as
closed-source precompiled static libraries for specific MCU toolchains, so no
CircuitPython driver exists. This test only verifies the BMV080 answers on the
I2C bus at 0x54. Use the [Arduino](../arduino/) or [ESPHome](../esphome/)
tests for actual PM readings.

## Running on hardware

1. Install CircuitPython 9+ on a board with a STEMMA QT/Qwiic connector (or
   wire the sensr-air board's SDA/SCL/3V3/GND to your board's I2C pins).
   The code prefers `board.STEMMA_I2C()` and falls back to
   `board.SCL`/`board.SDA`.
2. Install the Adafruit libraries onto the board with
   [circup](https://github.com/adafruit/circup):

   ```sh
   circup install adafruit_sgp41 adafruit_stcc4
   ```

   Add `adafruit_sgp40` too if you are testing a board older than 0.4.0.
   The SFA40 driver isn't in the Adafruit bundle, so copy its `sfa40.py` to
   the `lib` directory on the `CIRCUITPY` drive:

   ```sh
   git clone https://github.com/romkey/circuitpython-sfa40
   cp circuitpython-sfa40/sfa40.py /Volumes/CIRCUITPY/lib/
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
