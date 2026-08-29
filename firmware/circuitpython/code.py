# SPDX-License-Identifier: MIT
#
# Test firmware for the sensr-air board (CircuitPython)
#
# Sensors under test:
#   SGP40  (VOC,     I2C 0x59) - adafruit_sgp40
#   STCC4  (CO2,     I2C 0x64) - adafruit_stcc4
#   SHT40  (temp/RH) - not directly on the bus; read through the STCC4
#   BMV080 (PM,      I2C 0x57) - NO CircuitPython driver exists (Bosch only
#          ships closed-source static libraries), so this test only verifies
#          that the BMV080 responds on the I2C bus.
#
# Expected output: a PASS/FAIL line for each sensor, then continuous readings.

import time

import board
import busio

import adafruit_sgp40
import adafruit_stcc4

# Addresses on the sensr-air board
SGP40_ADDR = 0x59
STCC4_ADDR = 0x64  # 0x65 if the ADDR line is pulled high
BMV080_ADDR = 0x57

READ_INTERVAL = 2  # seconds between readings


def get_i2c():
    """Use the board's STEMMA/Qwiic bus if present, else the default I2C pins."""
    try:
        return board.STEMMA_I2C()
    except AttributeError:
        return busio.I2C(board.SCL, board.SDA)


def scan_bus(i2c):
    while not i2c.try_lock():
        pass
    try:
        found = i2c.scan()
    finally:
        i2c.unlock()
    return found


def report(name, ok, detail=""):
    status = "PASS" if ok else "FAIL"
    print("[%s] %s %s" % (status, name, detail))
    return ok


print("sensr-air CircuitPython sensor test")
print("=" * 50)

i2c = get_i2c()

devices = scan_bus(i2c)
print("I2C scan:", " ".join(hex(d) for d in devices) or "no devices found")

report("SGP40 present (0x%02x)" % SGP40_ADDR, SGP40_ADDR in devices)
report("STCC4 present (0x%02x)" % STCC4_ADDR, STCC4_ADDR in devices)
report(
    "BMV080 present (0x%02x)" % BMV080_ADDR,
    BMV080_ADDR in devices,
    "(presence check only - no CircuitPython driver)",
)
print()

# SGP40
sgp40 = None
try:
    # the driver verifies the SGP40's serial number during init
    sgp40 = adafruit_sgp40.SGP40(i2c, address=SGP40_ADDR)
    report("SGP40 init", True)
except Exception as e:
    report("SGP40 init", False, repr(e))

# STCC4 (+ SHT40 via the STCC4's private I2C bus)
stcc4 = None
try:
    stcc4 = adafruit_stcc4.STCC4(i2c, address=STCC4_ADDR)
    serial = "".join("%02x" % b for b in stcc4.serial_number)
    report("STCC4 init", True, "serial: %s" % serial)
    result = stcc4.self_test()
    report("STCC4 self test", result == 0, "result: %d" % result)
    stcc4.continuous_measurement = True
except Exception as e:
    report("STCC4 init", False, repr(e))
    stcc4 = None

print()
print("Continuous readings (every %ds):" % READ_INTERVAL)
print("-" * 50)

time.sleep(2)  # let the STCC4 finish its first measurement

while True:
    if sgp40:
        try:
            print(
                "SGP40:  raw %d, VOC index %d"
                % (sgp40.raw, sgp40.measure_index())
            )
        except Exception as e:
            print("SGP40:  read error:", repr(e))

    if stcc4:
        try:
            # temperature/relative_humidity come from the SHT40, which the
            # STCC4 reads over its private I2C bus
            print(
                "STCC4:  CO2 %d ppm | SHT40: %.1f C, %.1f %%RH"
                % (stcc4.CO2, stcc4.temperature, stcc4.relative_humidity)
            )
        except Exception as e:
            print("STCC4:  read error:", repr(e))

    print()
    time.sleep(READ_INTERVAL)
