#pragma once

// Copy this file to config.h (which is gitignored) and fill in your own
// network and hardware details before building.

// ---- Access point ----
// The controller hosts its own Wi-Fi network rather than joining an
// existing one -- WLED nodes connect to this AP directly. WPA2 requires
// the password to be at least 8 characters. The AP comes up at the
// ESP32 SoftAP default IP (192.168.4.1) with its own DHCP server, so
// nodes don't need static IPs configured.
#define AP_SSID "observatory-lighting"
#define AP_PASSWORD "change-this-password"
#define AP_CHANNEL 1
// ESP32 SoftAP supports at most ~10 simultaneous stations; keep some
// headroom above your actual node count.
#define AP_MAX_CONNECTIONS 8

// ---- WLED nodes ----
// Hostname or IP address of every WLED node this controller should drive.
// Configure each node's Wi-Fi Setup to join AP_SSID/AP_PASSWORD above (not
// your home network), and give each a unique mDNS hostname there -- see
// wled-nodes/README.md. mDNS resolution works the same way over this
// closed network as it would over a regular LAN.
static const char* const WLED_NODES[] = {
    "wled-node1.local",
    "wled-node2.local",
};
static const size_t WLED_NODE_COUNT = sizeof(WLED_NODES) / sizeof(WLED_NODES[0]);

// ---- Rotary encoder pins ----
#define PIN_ENCODER_A 32
#define PIN_ENCODER_B 33
#define PIN_ENCODER_BTN 25

// ---- Brightness ----
#define BRIGHTNESS_MIN 8
#define BRIGHTNESS_MAX 255
#define BRIGHTNESS_DEFAULT 128
// Every time Red mode is entered (including at boot, since Red is the
// default mode), brightness resets to this value -- 20% of 255 -- rather
// than carrying over whatever brightness was last set in another mode.
#define BRIGHTNESS_RED_DEFAULT 51
// Brightness change per detent. Most encoders produce 4 interrupt edges per
// detent; adjust ENCODER_PULSES_PER_STEP if your encoder differs.
#define BRIGHTNESS_STEP 8
#define ENCODER_PULSES_PER_STEP 4

// ---- Timing ----
#define BUTTON_DEBOUNCE_MS 40
#define ENCODER_SEND_INTERVAL_MS 120
#define HTTP_TIMEOUT_MS 1500
