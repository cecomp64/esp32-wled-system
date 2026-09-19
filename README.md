# esp32-wled-system

DIY LED lighting control for an astronomical observatory, built on ESP32 + WLED.

## Architecture

```
                 Controller's own Wi-Fi access point (2.4 GHz)
                     ┌───────────────────────────────────┐
                     │                                    │
  ┌──────────────┐   │   HTTP POST /json/state            │
  │  Controller   │───┼──────────────► ┌────────────────┐ │
  │  ESP32        │   │                 │ WLED node #1   │ │
  │  (hosts AP)   │───┼──────────────►  │ ESP32 + WS2815 │ │
  │ - rotary      │   │                 └────────────────┘ │
  │   encoder     │───┼──────────────► ┌────────────────┐ │
  │   (brightness)│   │                 │ WLED node #2   │ │
  │ - push button │───┼──────────────►  │ ESP32 + WS2815 │ │
  │   (mode/on-off)│  │                 └────────────────┘ │
  └──────────────┘   │        ...  (one HTTP POST per      │
                      │             node, per event)        │
                      └───────────────────────────────────┘
```

- **WLED nodes** (`wled-nodes/`): one or more ESP32 boards, each running stock
  [WLED](https://kno.wled.ge/) firmware and driving its own run of addressable
  WS2815 LEDs. These are configured, not custom-coded — WLED already speaks a
  JSON HTTP API that does everything we need.
- **Controller** (`controller/`): a separate ESP32 with a rotary encoder
  (with integrated push-button) wired to it. This is custom firmware (this
  repo's only compiled code) that reads the encoder/button and pushes state
  changes to every WLED node over Wi-Fi using WLED's JSON API.

There is no central hub/server beyond the controller itself — it talks
directly to each WLED node's HTTP API. The controller **hosts its own Wi-Fi
access point** (SoftAP) rather than joining an existing network; every WLED
node connects to that AP. This keeps the whole system self-contained (no
dependency on a home router's signal reaching the observatory), at the cost
of the nodes having no general internet access (no NTP, no cloud OTA). A
16x20ft room is trivial range for the ESP32's SoftAP radio. See
`controller/README.md` for the AP's default IP/credentials and
`wled-nodes/README.md` for pointing each node at it.

## Behavior

- **Rotary rotation** → adjusts brightness (applied to all nodes).
- **Button press** → cycles mode: `Red` → `White` → `Off` → `Red` → ...
  - `Red`: full-brightness red, for preserving night/dark adaptation at the
    eyepiece. This is the default/boot mode.
  - `White`: full white work light.
  - `Off`: all nodes off.

See `controller/README.md` for firmware details and `wled-nodes/README.md`
for how to set up each LED node.

## Hardware (per node)

- ESP32 dev board
- WS2815 addressable LED strip (12V, RGB, dual data line for redundancy)
- 12V power supply sized for the strip length, injected at both ends of long
  runs
- Level shifting is usually **not** required for WS2815 (its data-high
  threshold tolerates the ESP32's 3.3V logic at short wire lengths), but add
  a 3.3V→5V level shifter on the data line if you see flicker/glitches,
  especially on long data runs.
- A ~300-500Ω resistor in series with the data line and a large (~1000µF)
  capacitor across +/GND at the strip input are recommended.

## Hardware (controller)

- ESP32 dev board
- Rotary encoder with push-button (e.g. KY-040 or similar), 5 pins: `A`,
  `B`, `SW` (button), `+`, `GND`

## Repo layout

```
controller/     PlatformIO project for the rotary-encoder/button controller
wled-nodes/     WLED configuration templates + setup guide for the LED nodes
```

## Possible future extensions

- Additional modes (e.g. dim red for star parties, a "flat field" white
  panel mode) — add a case to the mode cycle in `controller/src/main.cpp`.
- Replace HTTP polling-per-node with WLED's UDP sync/broadcast protocol for
  tighter multi-node sync.
- OTA firmware updates for the controller.
- Long-press on the encoder button for a config portal (e.g. WiFiManager)
  to change the AP SSID/password without reflashing.
