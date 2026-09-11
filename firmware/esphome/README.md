# ESPHome Test Firmware

Test firmware for the sensr-air board sensors as an ESPHome configuration,
targeting ESP32-S3 (adjust `board` and I2C pins in `sensor-test.yaml` for
your dev board).

## What it tests

| Sensor | Address | Component | Coverage |
|--------|---------|-----------|----------|
| SGP41 (VOC/NOx) | 0x59 | native [`sgp4x`](https://esphome.io/components/sensor/sgp4x/) platform | VOC index + NOx index |
| STCC4 (CO2) | 0x64 | local external component ([components/stcc4](components/stcc4/)) | CO2 readings |
| SFA40 (HCHO) | 0x5D | native `sfa40` platform (ESPHome 2026.9+) | formaldehyde, temp/RH |
| SHT40 (temp/RH) | (via STCC4) | local external component | temp/RH read through the STCC4 |
| BMV080 (PM) | 0x54 | [sweitzja/esphome-bmv080](https://github.com/sweitzja/esphome-bmv080) | PM1 / PM2.5 / PM10, obstruction |

The SHT40 is not directly reachable on the I2C bus — it sits on the STCC4's
private second I2C bus, and the STCC4 reports its temperature and humidity
values as part of its measurement data.

The SFA40 measures its own temperature and humidity and uses them to
compensate the formaldehyde reading, so it reports all three.

### SGP40 or SGP41?

Boards from `hw/air-stcc4/0.4.0` on carry an SGP41 (VOC **and** NOx); earlier
boards carry an SGP40 (VOC only). You don't have to configure which: both
answer at 0x59, and the `sgp4x` component reads the sensor's feature set
(command 0x202F) at startup and logs which part it found. NOx needs an SGP41 —
on an SGP40 the component logs `SGP41 required for NOx, disabling NOx sensor`
and carries on with VOC alone, so this config works on either board.

### ESPHome version

The `sfa40` component was merged upstream after the 2026.8 release, so both
the CI workflow and `docker-compose.yml` build against the ESPHome **beta**
channel. Switch them back to `latest` once a stable release includes it.

ESPHome has no upstream STCC4 support yet, so this directory ships a small
external component in [components/stcc4](components/stcc4/) that starts
continuous measurement and reads CO2, temperature, and humidity using
Sensirion's documented I2C command set.

The BMV080 component wraps Bosch's closed-source precompiled SDK; it supports
ESP32, ESP32-S3, ESP32-C3, and ESP32-C6. It injects the SDK through
PlatformIO build flags, so the config pins `toolchain: platformio` — ESPHome's
native ESP-IDF toolchain (the default since 2026.7) doesn't honor those flags
and the build fails without it.

## Building

```sh
cp secrets.yaml.example secrets.yaml   # then edit with your Wi-Fi credentials
esphome compile sensor-test.yaml
```

Or with docker compose:

```sh
cp secrets.yaml.example secrets.yaml
docker compose run --rm compile
```

## Flashing and logs

```sh
esphome run sensor-test.yaml
```

Or with docker compose (adjust the serial device in `docker-compose.yml` if
needed):

```sh
docker compose run --rm esphome run sensor-test.yaml
```

The log output shows the I2C scan (expect devices at 0x54, 0x59, 0x5D, and
0x64) and periodic readings from all sensors.

If the STCC4's ADDR line is pulled high, change its `address` to `0x65` in
`sensor-test.yaml`.
