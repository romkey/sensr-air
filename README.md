# sensr-air

[![CircuitPython](https://github.com/romkey/sensr-air/actions/workflows/circuitpython.yml/badge.svg)](https://github.com/romkey/sensr-air/actions/workflows/circuitpython.yml)
[![Arduino](https://github.com/romkey/sensr-air/actions/workflows/arduino.yml/badge.svg)](https://github.com/romkey/sensr-air/actions/workflows/arduino.yml)
[![ESPHome](https://github.com/romkey/sensr-air/actions/workflows/esphome.yml/badge.svg)](https://github.com/romkey/sensr-air/actions/workflows/esphome.yml)
[![KiCAD](https://github.com/romkey/sensr-air/actions/workflows/kicad.yml/badge.svg)](https://github.com/romkey/sensr-air/actions/workflows/kicad.yml)
[![kilint](https://github.com/romkey/sensr-air/actions/workflows/kilint.yml/badge.svg)](https://github.com/romkey/sensr-air/actions/workflows/kilint.yml)
[![BOFFF](https://github.com/romkey/sensr-air/actions/workflows/bofff.yml/badge.svg)](https://github.com/romkey/sensr-air/actions/workflows/bofff.yml)

Hardware design and test firmware for a small air quality sensor PCB with:

| Sensor | Function | I2C address |
|--------|----------|-------------|
| Sensirion SGP40 | VOC index | 0x59 |
| Sensirion STCC4 | CO2 | 0x64 (0x65 if ADDR is high) |
| Sensirion SHT40 | temperature / humidity | none — connected to the STCC4's private I2C bus and read through the STCC4 |
| Bosch BMV080 | PM1 / PM2.5 / PM10 | 0x57 |

This is a 3.3 V board with no voltage regulator — don't use it with 5 V
circuits.

## Hardware

Board designs live under [hw/](hw/), one subdirectory per board with a
subdirectory per version:

- [hw/air-stcc4/](hw/air-stcc4/) — STCC4 + SHT40 breakout
- [hw/air/](hw/air/) — earlier SCD40 + BME680 board

### KiCAD CI

The [KiCAD workflow](.github/workflows/kicad.yml) runs
[ci/kicad-check.sh](ci/kicad-check.sh), which runs ERC on every schematic and
DRC on every PCB of the latest version of each board under `hw/`. Only errors
fail the check; warnings are ignored. Boards or versions containing a
`.ci-kicad-ignore` file are skipped.

Run it locally with KiCAD 8+ installed:

```sh
./ci/kicad-check.sh
```

### kilint CI

The [kilint workflow](.github/workflows/kilint.yml) runs
[kilint](https://github.com/romkey/kilint), a linter for KiCAD project
conventions that ERC and DRC don't cover (title blocks, reference designators,
mounting holes, part number fields, net classes, and so on). It runs only when
[BOFFF.md](BOFFF.md) is present, and lints the single board that BOFFF.md's
`repository_path` points at via [ci/kilint-check.sh](ci/kilint-check.sh).
Older boards under `hw/` predate these conventions and are not checked.

Rule configuration lives in [.kilint.yml](.kilint.yml). kilint has no tagged
releases yet, so the workflow installs it from `main`; pin `KILINT_REF` in the
workflow once it does.

Run it locally:

```sh
pip install git+https://github.com/romkey/kilint
./ci/kilint-check.sh
```

### BOFFF

[BOFFF.md](BOFFF.md) is this board's machine-readable fabrication/form-factor
description. The [BOFFF workflow](.github/workflows/bofff.yml) does nothing but
assert the file exists, so the BOFFF badge above is green whenever the repo
carries one.

## Test firmware

Each firmware directory contains a test program that exercises all of the
board's sensors, a README, and (where possible) a docker compose file to
build it. All of them print/report readings from each sensor so you can
verify a freshly assembled board.

| Directory | Platform | SGP40 | STCC4 | SHT40 (via STCC4) | BMV080 |
|-----------|----------|-------|-------|-------------------|--------|
| [firmware/circuitpython/](firmware/circuitpython/) | CircuitPython 9+ | yes | yes | yes | presence check only¹ |
| [firmware/arduino/](firmware/arduino/) | Arduino / ESP32 | yes | yes | yes | yes |
| [firmware/esphome/](firmware/esphome/) | ESPHome / ESP32 | yes | yes² | yes² | yes |

¹ Bosch only ships the BMV080 driver as closed-source precompiled static
libraries for specific MCU toolchains, so no CircuitPython driver exists.

² ESPHome has no upstream STCC4 support; this repo includes a local
[external component](firmware/esphome/components/stcc4/) for it.

CI builds all three firmwares on every push
([CircuitPython](.github/workflows/circuitpython.yml),
[Arduino](.github/workflows/arduino.yml),
[ESPHome](.github/workflows/esphome.yml)).

## License

Hardware designs are licensed under the
[CERN Open Hardware Licence v2 Permissive](https://ohwr.org/cern_ohl_p_v2.txt).
Firmware is licensed under the [MIT License](https://opensource.org/license/mit).
