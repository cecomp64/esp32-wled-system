# controller

PlatformIO firmware for the observatory lighting controller: a standalone
ESP32 wired to a rotary encoder (with integrated push-button) that drives
one or more WLED nodes over Wi-Fi via WLED's JSON HTTP API. The controller
hosts its own Wi-Fi access point (it does not join an existing network) --
WLED nodes connect to it directly.

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

1. Copy the example config and fill in your access point SSID/password and
   the hostname/IP of every WLED node to control:

   ```sh
   cp include/config.example.h include/config.h
   ```

   `config.h` is gitignored so your credentials never get committed.

2. Build and upload with [PlatformIO](https://platformio.org/):

   ```sh
   pio run -t upload
   pio device monitor
   ```

3. Flash/configure each WLED node to join the AP you just set (see
   `wled-nodes/README.md`) instead of your home network. The controller
   comes up at `192.168.4.1` (the ESP32 SoftAP default) once it boots.

## Behavior

- Boots into **Red** mode, and pushes that state to every configured node.
- **Entering Red mode** (at boot, or by cycling back to it) always resets
  brightness to `BRIGHTNESS_RED_DEFAULT` (20%), regardless of whatever
  brightness White/Off last used.
- **Rotating the encoder** adjusts brightness by `BRIGHTNESS_STEP` per
  detent (clamped to `BRIGHTNESS_MIN`..`BRIGHTNESS_MAX`), throttled to at
  most one HTTP push per node every `ENCODER_SEND_INTERVAL_MS` so a fast
  spin doesn't flood the network.
- **Pressing the button** cycles mode: `Red` → `White` → `Off` → `Red` ...
  Each mode change is pushed to every node immediately.
- On boot, the firmware starts its own access point (`AP_SSID`/
  `AP_PASSWORD` in `config.h`) rather than joining an existing network.
  There's no reconnect logic to worry about the way there would be for a
  station joining someone else's Wi-Fi; the AP just stays up. A node that
  isn't connected yet (or drops off) simply misses updates until it
  reconnects and gets the next mode/brightness change.
- After the AP comes up, the firmware calls `MDNS.begin(...)` once so
  `.local` hostnames in `WLED_NODES` resolve reliably (without this,
  ESP32's HTTP client isn't guaranteed to resolve `.local` names) -- this
  works the same way over the AP's own network as it would over a joined
  one. This also makes the controller itself discoverable/pingable as
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
- ESP32 SoftAP supports at most ~10 simultaneous stations (hardware
  limit). `AP_MAX_CONNECTIONS` in `config.h` defaults to 8; if you have
  more WLED nodes than that, raise it (up to ~10) or split nodes across
  more than one AP.
