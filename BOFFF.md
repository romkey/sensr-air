---
bofff_version: 1
board:
  name: Sensr Air CO2/HCHO/PM/VOC
  version: 0.4.0
  date: '2026-08-28'
  organization: PDX Hackerspace
  license: CERN-OHL-S
  repository: https://github.com/romkey/sensr-air
  repository_path: hw/air-stcc4/0.4.0
  notes:
  - John Romkey
  - 'License: CERN-OHL-S https://ohwr.org/cern_ohl_s_v2.pdf'
source_files:
  ecad: KiCad
  project: air-stcc4.kicad_pro
  schematic: air-stcc4.kicad_sch
  pcb: air-stcc4.kicad_pcb
  step: null
  dxf: null
  bom: null
form:
  units: mm
  origin: top-left of bounding box, x right, y down
  outline:
    shape: rectangle
    width: 50
    height: 35
  thickness: 1.6
  copper_layers: 2
  max_height_top: null
  max_height_bottom: null
mounting:
  holes:
  - ref: MNT1
    x: 2.5
    y: 2.5
    diameter: 2.2
  - ref: MNT2
    x: 47.5
    y: 2.5
    diameter: 2.2
  - ref: MNT3
    x: 2.5
    y: 32.5
    diameter: 2.2
  - ref: MNT4
    x: 47.5
    y: 32.5
    diameter: 2.2
power:
  rails:
  - net: GND
    kind: ground
    connections: 14
  - net: +3.3V
    kind: supply
    connections: 16
  usb:
    present: false
  battery:
    present: false
  input_voltage: null
  current_typical: null
  current_peak: null
connectors:
- ref: FPC1
  part: 046844713002846+_C5857733
  footprint: easyeda2kicad:FPC-SMD_13P-P0.60_046844713002846-1
  x: 10.555
  y: 7.124
  populated: true
  pins:
  - pad: '2'
    net: /BV_IRQ
  - pad: '3'
    net: GND
  - pad: '4'
    net: +3.3V
  - pad: '5'
    net: GND
  - pad: '6'
    net: +3.3V
  - pad: '7'
    net: +3.3V
  - pad: '8'
    net: /SCL
  - pad: '9'
    net: /SDA
  - pad: '10'
    net: GND
  - pad: '11'
    net: +3.3V
  - pad: '12'
    net: GND
  - pad: '13'
    net: +3.3V
  - pad: '14'
    net: GND
  - pad: '15'
    net: GND
- ref: J1
  part: BM04B-SRSS-TB(LF)(SN)
  footprint: easyeda2kicad:CONN-SMD_4P-P1.00_SM04B-SRSS-TB-LF-SN
  x: 3.5
  y: 21
  populated: true
  pins:
  - pad: '1'
    net: GND
  - pad: '2'
    net: +3.3V
  - pad: '3'
    net: /SDA
  - pad: '4'
    net: /SCL
  - pad: '5'
    net: GND
  - pad: '6'
    net: GND
- ref: J2
  part: BM04B-SRSS-TB(LF)(SN)
  footprint: easyeda2kicad:CONN-SMD_4P-P1.00_SM04B-SRSS-TB-LF-SN
  x: 46.31
  y: 21
  populated: true
  pins:
  - pad: '1'
    net: GND
  - pad: '2'
    net: +3.3V
  - pad: '3'
    net: /SDA
  - pad: '4'
    net: /SCL
  - pad: '5'
    net: GND
  - pad: '6'
    net: GND
- ref: J3
  part: Conn_01x08
  footprint: Connector_PinSocket_2.54mm:PinSocket_1x05_P2.54mm_Vertical
  x: 11.42
  y: 33
  populated: false
  pins:
  - pad: '1'
    net: +3.3V
  - pad: '2'
    net: GND
  - pad: '3'
    net: /SCL
  - pad: '4'
    net: /SDA
  - pad: '5'
    net: /BV_IRQ
interfaces:
  i2c:
  - name: primary
    nets:
    - /SCL
    - /SDA
    pullups:
    - ref: R4
      line: SCL
      net: /SCL
      pulls_to: +3.3V
      voltage: 3.3
      value: 2.2K
      resistance: 2200
    - ref: R3
      line: SDA
      net: /SDA
      pulls_to: +3.3V
      voltage: 3.3
      value: 2.2K
      resistance: 2200
    devices:
    - ref: U1
      part: SGP41
      address: '0x59'
    - ref: U2
      part: STCC4
      address: '0x64'
    - ref: U3
      part: SFA40-D-R1
      address: '0x5D'
    exposed_on:
    - FPC1
    - J1
    - J2
    - J3
  - name: _c
    nets:
    - /SCL_C
    - /SDA_C
    pullups: null
    devices:
    - ref: U2
      part: STCC4
      address: '0x64'
    - ref: U4
      part: SHT40
      address: '0x44'
    exposed_on: null
---

# Sensr Air CO2/HCHO/PM/VOC — BOFFF

**BOFFF** — a Bill Of Form, Fit and Function. A summary of this
board's physical and electrical facts for anyone designing an
enclosure, a mount, or a mating board.

- **Form** — outline, thickness, layer count.
- **Fit** — mounting holes and connector positions.
- **Function** — power rails and the buses the board exposes.

## Coordinates

Millimetres. The origin is the **top-left corner of the board
outline's bounding box**, with X increasing right and Y increasing
**down**. This is the opposite of the usual mathematical
convention and is the most likely source of a silent error in
anything consuming this file.

## Generated, not authored

Everything above was extracted from the KiCad files listed under
`source_files`. Regenerate it rather than editing it by hand:

```bash
kilint bofff . > BOFFF.md
```

For exact geometry — curves, slots, internal cutouts, component
heights — use the STEP/DXF exports rather than this summary. A
bounding box is not an outline.

## Complete by hand

These are not recorded in the KiCad files and are emitted as
`null`. They need a human:

- `form.max_height_top` — tallest component above the board; from the 3D models, not the PCB
- `form.max_height_bottom` — tallest component below the board; same
- `power.input_voltage` — the operating range; the netlist does not say
- `power.current_typical` — a measurement
- `power.current_peak` — a measurement
- `source_files.step` — path to the 3D export, once you make one
- `source_files.dxf` — path to the 2D outline export, once you make one
- `source_files.bom` — path to the exported BOM

