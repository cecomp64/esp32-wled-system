# controller

PlatformIO firmware for the observatory lighting controller: a standalone
ESP32 wired to a rotary encoder (with integrated push-button) that drives
one or more WLED nodes over Wi-Fi via WLED's JSON HTTP API.

## Wiring

| Encoder pin | ESP32 pin (default, see `config.h`) |
|---|---|
| `A`   | GPIO32 |
| `B`   | GPIO33 |
| `SW` (button) | GPIO25 |
| `+`   | 3.3V |
| `GND` | GND |

All three signal pins are configured `INPUT_PULLUP`; a common KY-040-style
module works directly with no external resistors. The button reads
active-low (pressed = LOW).

## Setup

1. Copy the example config and fill in your Wi-Fi credentials and the
   hostname/IP of every WLED node to control:

   ```sh
   cp include/config.example.h include/config.h
   ```

   `config.h` is gitignored so your credentials never get committed.

2. Build and upload with [PlatformIO](https://platformio.org/):

   ```sh
   pio run -t upload
   pio device monitor
   ```

## Behavior

- Boots into **Red** mode at `BRIGHTNESS_DEFAULT`, and pushes that state to
  every configured node.
- **Rotating the encoder** adjusts brightness by `BRIGHTNESS_STEP` per
  detent (clamped to `BRIGHTNESS_MIN`..`BRIGHTNESS_MAX`), throttled to at
  most one HTTP push per node every `ENCODER_SEND_INTERVAL_MS` so a fast
  spin doesn't flood the network.
- **Pressing the button** cycles mode: `Red` → `White` → `Off` → `Red` ...
  Each mode change is pushed to every node immediately.
- If Wi-Fi drops, the main loop reconnects automatically; state pushes are
  silently skipped while disconnected.

## Notes

- Each button/encoder event does a blocking HTTP POST to every node in
  turn. For a handful of nodes on a local network this is fast enough to
  feel instant; if you add many nodes and it becomes noticeably slow,
  switch to WLED's UDP sync protocol instead (see the top-level README's
  "Possible future extensions").
- `ENCODER_PULSES_PER_STEP` assumes a typical 4-edge-per-detent encoder;
  if a full turn's worth of clicks feels like 2x or 4x too much/too little
  brightness change, adjust this value.
