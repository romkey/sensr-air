# SPDX-License-Identifier: MIT
#
# Test firmware for the sensr-air board (CircuitPython)
#
# Sensors under test (addresses as populated on hw/air-stcc4/0.4.0):
#   SGP41  (VOC/NOx, I2C 0x59) - adafruit_sgp41; boards up to 0.3.0 carry an
#          SGP40 instead, which is detected at runtime (see sgp4x_variant)
#   STCC4  (CO2,     I2C 0x64) - adafruit_stcc4
#   SFA40  (HCHO,    I2C 0x5D) - sfa40 (romkey/circuitpython-sfa40)
#   SHT40  (temp/RH) - not directly on the bus; it sits on the STCC4's private
#          I2C bus and is read through the STCC4
#   BMV080 (PM,      I2C 0x54) - NO CircuitPython driver exists (Bosch only
#          ships closed-source static libraries), so this test only verifies
#          that the BMV080 responds on the I2C bus.
#
# Expected output: a PASS/FAIL line for each sensor, then continuous readings.

import time

import board
import busio

import adafruit_stcc4
import sfa40

# Addresses on the sensr-air board
SGP4X_ADDR = 0x59
STCC4_ADDR = 0x64  # 0x65 if the ADDR line is pulled high
SFA40_ADDR = sfa40.SFA40_DEFAULT_ADDRESS  # 0x5D
BMV080_ADDR = 0x54

# SGP4x "Get Feature Set" (0x202F). The low 9 bits of the reply identify the
# part: the SGP40 measures VOC only, the SGP41 measures VOC and NOx. Both sit
# at 0x59 and answer the same serial-number command, so the feature set is the
# only way to tell them apart.
SGP4X_CMD_GET_FEATURESET = b"\x20\x2f"
SGP40_FEATURESET = 0x0020
SGP41_FEATURESET = 0x0040

READ_INTERVAL = 2  # seconds between readings
SGP41_CONDITIONING_S = 10  # NOx conditioning; must not exceed 10s


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


def crc8(data):
    """Sensirion's CRC-8 (polynomial 0x31, init 0xFF)."""
    crc = 0xFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc << 1) ^ 0x31) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
    return crc


def sgp4x_variant(i2c):
    """Return "SGP41", "SGP40", or None, by reading the sensor's feature set."""
    buf = bytearray(3)
    while not i2c.try_lock():
        pass
    try:
        i2c.writeto(SGP4X_ADDR, SGP4X_CMD_GET_FEATURESET)
        time.sleep(0.01)
        i2c.readfrom_into(SGP4X_ADDR, buf)
    except OSError:
        return None
    finally:
        i2c.unlock()

    if crc8(buf[0:2]) != buf[2]:
        return None

    featureset = ((buf[0] << 8) | buf[1]) & 0x1FF
    if featureset == SGP41_FEATURESET:
        return "SGP41"
    if featureset == SGP40_FEATURESET:
        return "SGP40"
    return None


def report(name, ok, detail=""):
    status = "PASS" if ok else "FAIL"
    print("[%s] %s %s" % (status, name, detail))
    return ok


print("sensr-air CircuitPython sensor test")
print("=" * 50)

i2c = get_i2c()

devices = scan_bus(i2c)
print("I2C scan:", " ".join(hex(d) for d in devices) or "no devices found")

variant = sgp4x_variant(i2c) if SGP4X_ADDR in devices else None
report(
    "SGP4x present (0x%02x)" % SGP4X_ADDR,
    variant is not None,
    "detected: %s" % (variant or "unknown"),
)
report("STCC4 present (0x%02x)" % STCC4_ADDR, STCC4_ADDR in devices)
report("SFA40 present (0x%02x)" % SFA40_ADDR, SFA40_ADDR in devices)
report(
    "BMV080 present (0x%02x)" % BMV080_ADDR,
    BMV080_ADDR in devices,
    "(presence check only - no CircuitPython driver)",
)
print()

# SGP41 (or SGP40 on boards up to 0.3.0)
sgp = None
if variant == "SGP41":
    try:
        from adafruit_sgp41.sgp41 import SGP41

        sgp = SGP41(i2c, address=SGP4X_ADDR)
        report("SGP41 self test", sgp.self_test_passed)
    except Exception as e:  # noqa: BLE001 - report and keep testing other sensors
        report("SGP41 init", False, repr(e))
        sgp = None
elif variant == "SGP40":
    try:
        import adafruit_sgp40

        # the driver verifies the SGP40's serial number during init
        sgp = adafruit_sgp40.SGP40(i2c, address=SGP4X_ADDR)
        report("SGP40 init", True)
    except Exception as e:  # noqa: BLE001
        report("SGP40 init", False, repr(e))
        sgp = None

# STCC4 (+ SHT40 via the STCC4's private I2C bus)
stcc4 = None
try:
    stcc4 = adafruit_stcc4.STCC4(i2c, address=STCC4_ADDR)
    serial = "".join("%02x" % b for b in stcc4.serial_number)
    report("STCC4 init", True, "serial: %s" % serial)
    result = stcc4.self_test()
    report("STCC4 self test", result == 0, "result: %d" % result)
    stcc4.continuous_measurement = True
except Exception as e:  # noqa: BLE001
    report("STCC4 init", False, repr(e))
    stcc4 = None

# SFA40 (formaldehyde, plus the humidity/temperature it compensates with).
# The constructor starts continuous measurement. The SFA40's self-test puts the
# sensor into a special mode for 5-6 minutes, so it is not run here - see the
# driver's sfa40_self_test.py example if you want to run it after assembly.
sfa = None
try:
    sfa = sfa40.SFA40(i2c, address=SFA40_ADDR)
    report("SFA40 init", True, "serial: %s" % sfa.serial_number_string)
except Exception as e:  # noqa: BLE001
    report("SFA40 init", False, repr(e))
    sfa = None

# The SGP41 needs up to 10s of NOx conditioning at 1Hz before its first real
# measurement. The SGP40 has no conditioning phase.
if variant == "SGP41" and sgp:
    print()
    print("Conditioning the SGP41 (%ds)..." % SGP41_CONDITIONING_S)
    for _ in range(SGP41_CONDITIONING_S):
        try:
            sgp.conditioning()
        except Exception as e:  # noqa: BLE001
            print("SGP41: conditioning error:", repr(e))
            break
        time.sleep(1)

print()
print("Continuous readings (every %ds):" % READ_INTERVAL)
print("-" * 50)

# let the STCC4 and SFA40 finish their first measurements
time.sleep(max(2, sfa40.FIRST_MEASUREMENT_DELAY))

while True:
    if sgp and variant == "SGP41":
        try:
            # the gas index algorithm expects 1Hz sampling and returns 0 for
            # both indices while it warms up (about 45s)
            voc_index, nox_index = sgp.measure_index()
            print(
                "SGP41:  raw VOC %d, raw NOx %d | VOC index %d, NOx index %d"
                % (sgp.raw_voc, sgp.raw_nox, voc_index, nox_index)
            )
        except Exception as e:  # noqa: BLE001
            print("SGP41:  read error:", repr(e))
    elif sgp:
        try:
            print("SGP40:  raw %d, VOC index %d" % (sgp.raw, sgp.measure_index()))
        except Exception as e:  # noqa: BLE001
            print("SGP40:  read error:", repr(e))

    if stcc4:
        try:
            # temperature/relative_humidity come from the SHT40, which the
            # STCC4 reads over its private I2C bus
            print(
                "STCC4:  CO2 %d ppm | SHT40: %.1f C, %.1f %%RH"
                % (stcc4.CO2, stcc4.temperature, stcc4.relative_humidity)
            )
        except Exception as e:  # noqa: BLE001
            print("STCC4:  read error:", repr(e))

    if sfa:
        try:
            m = sfa.measurement
            # the SFA40 measures its own humidity/temperature and uses them to
            # compensate the formaldehyde signal
            print(
                "SFA40:  HCHO %.1f ppb | %.1f C, %.1f %%RH%s"
                % (
                    m.formaldehyde,
                    m.temperature,
                    m.relative_humidity,
                    "" if sfa.within_specification else "  (warming up)",
                )
            )
        except Exception as e:  # noqa: BLE001
            print("SFA40:  read error:", repr(e))

    print()
    time.sleep(READ_INTERVAL)
