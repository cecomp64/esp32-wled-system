# wled-nodes

Each LED node is a plain ESP32 running stock [WLED](https://kno.wled.ge/)
firmware, driving one run of WS2815 addressable LEDs. There's no custom code
here — WLED already exposes the JSON HTTP API the `controller/` firmware
talks to. This directory just documents how to flash and configure a node,
plus a couple of config templates you can adapt.

## Wiring (per node)

- WS2815 is a 12V strip with a single data line (the second "backup" data
  wire on some WS2815 variants is for redundancy against a dead first LED,
  not a second logical signal — tie both data-in pads together if your
  strip has two).
- Data pin: any usable ESP32 GPIO (WLED default is GPIO2; the controller
  doesn't care which pin each node uses since it's all behind the node's
  own HTTP API).
- Add a ~300-500Ω resistor in series on the data line close to the ESP32,
  and a ~1000µF capacitor across +12V/GND at the strip's power input.
- Power the ESP32 and the strip from a common ground. Inject 12V at both
  ends of long runs to avoid voltage-drop color shift.

## Flashing WLED

1. Flash the latest WLED release using the
   [web installer](https://install.wled.me) (Chrome/Edge, USB) or
   `esptool.py` with a downloaded `.bin` for the `esp32` target.
2. On first boot, WLED starts an AP named `WLED-AP` — connect to it and
   enter the **controller's** access point credentials (`AP_SSID`/
   `AP_PASSWORD` from `controller/include/config.h`), not your home
   network. The controller must already be powered on and running its
   firmware for the node to join it. The node gets an IP from the
   controller's DHCP server (in the `192.168.4.x` range).
3. Open the node's IP (or `http://wled-<name>.local` if mDNS resolves on
   your network) and go to **Config > LED Preferences**:
   - LED count: match your strip.
   - LED type: `WS281x` (WS2815 uses the same protocol timing as
     WS2812/WS2811 — there is no separate "WS2815" entry in most WLED
     builds; just set the correct GPIO and length).
   - Color order: `GRB` (verify against a short test — swap to `RGB`/`BRG`
     if colors look wrong).
   - Max current: set per your PSU/strip so WLED's built-in current limiter
     doesn't let you overdrive the wiring.
4. **Config > WiFi Setup**: set a unique, memorable hostname (e.g.
   `wled-node1`), matching what you put in the controller's
   `WLED_NODES` list. Enable mDNS if your network supports it — it works
   the same way on the controller's AP network as it would on a home LAN.
5. **Config > Security & Updates**: consider disabling OTA if not needed,
   and set an admin password if the network isn't fully trusted.

Repeat for each node, giving each a distinct hostname.

Note: since the controller's AP is the only network here, nodes have no
path to the internet — WLED's own OTA-from-URL and NTP time sync won't
work. Neither is needed for the controller/node flows this repo uses.

## Config templates

- `cfg.example.json` — a trimmed example of WLED's `cfg.json`
  (Config > Backup & Restore) showing the fields worth reviewing per node
  (LED count/pin/order, hostname). Edit the placeholders and import it via
  **Config > Backup & Restore > Restore Configuration** to speed up setting
  up additional nodes, or just use it as a reference for the UI fields.
- `presets.example.json` — optional WLED presets ("Red", "White") you can
  import via **Config > Backup & Restore > Restore Presets** so the same
  looks are also reachable from WLED's own app/UI, independent of the
  controller. The controller itself does not use presets — it pushes raw
  `on`/`bri`/`seg` state directly, so it works even if you never import
  these.

## Verifying a node independently of the controller

Each node's JSON API can be exercised directly, which is useful for
bring-up before wiring in the controller:

```sh
# solid red, full brightness
curl -s -X POST http://wled-node1.local/json/state \
  -H 'Content-Type: application/json' \
  -d '{"on":true,"bri":255,"seg":[{"fx":0,"col":[[255,0,0]]}]}'

# off
curl -s -X POST http://wled-node1.local/json/state \
  -H 'Content-Type: application/json' -d '{"on":false}'
```
