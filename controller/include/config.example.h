#pragma once

// Copy this file to config.h (which is gitignored) and fill in your own
// network and hardware details before building.

// ---- Wi-Fi ----
#define WIFI_SSID "your-ssid"
#define WIFI_PASSWORD "your-password"

// ---- WLED nodes ----
// Hostname or IP address of every WLED node this controller should drive.
// mDNS hostnames (the default WLED sets, e.g. "wled-<name>.local") work as
// long as your router/network supports mDNS resolution from the ESP32.
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
// Brightness change per detent. Most encoders produce 4 interrupt edges per
// detent; adjust ENCODER_PULSES_PER_STEP if your encoder differs.
#define BRIGHTNESS_STEP 8
#define ENCODER_PULSES_PER_STEP 4

// ---- Timing ----
#define BUTTON_DEBOUNCE_MS 40
#define ENCODER_SEND_INTERVAL_MS 120
#define HTTP_TIMEOUT_MS 1500
