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
- On boot, after Wi-Fi connects, the firmware calls `MDNS.begin(...)` once
  so `.local` hostnames in `WLED_NODES` resolve reliably (without this,
  ESP32's HTTP client isn't guaranteed to resolve `.local` names). This
  also makes the controller itself discoverable/pingable as
  `wled-controller.local`, though nothing currently depends on that.

## Notes

- HTTP sends run on a dedicated FreeRTOS task pinned to the core `loop()`
  doesn't use, fed by a small queue. Button and encoder polling (`loop()`,
  core 1) never blocks on network I/O, so a slow or unreachable node
  (each request can take up to `HTTP_TIMEOUT_MS`) doesn't make the knob or
  button feel unresponsive. The queue holds a handful of pending updates
  (`xQueueCreate(8, ...)`); if it's ever full — sustained updates faster
  than the network task can drain them, e.g. several nodes all timing out
  at once — the newest update is dropped and logged rather than blocking.
  For a handful of nodes on a local network this essentially never
  happens; if you add many nodes and drops become frequent, switch to
  WLED's UDP sync protocol instead (see the top-level README's "Possible
  future extensions").
- `ENCODER_PULSES_PER_STEP` assumes a typical 4-edge-per-detent encoder;
  if a full turn's worth of clicks feels like 2x or 4x too much/too little
  brightness change, adjust this value.
