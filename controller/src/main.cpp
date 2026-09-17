// Observatory lighting controller.
//
// Reads a rotary encoder (brightness) and its push-button (mode / on-off)
// and pushes the resulting state to every configured WLED node over the
// JSON HTTP API (POST /json/state).

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "config.h"

enum Mode : uint8_t {
  MODE_RED = 0,
  MODE_WHITE,
  MODE_OFF,
  MODE_COUNT
};

static Mode currentMode = MODE_RED;
static uint8_t brightness = BRIGHTNESS_DEFAULT;

// --- Encoder state (updated from ISR, consumed from loop) ---
static volatile int32_t encoderAccum = 0;
static volatile uint8_t lastEncoded = 0;

void IRAM_ATTR encoderISR() {
  uint8_t msb = digitalRead(PIN_ENCODER_A);
  uint8_t lsb = digitalRead(PIN_ENCODER_B);
  uint8_t encoded = (msb << 1) | lsb;
  uint8_t sum = (lastEncoded << 2) | encoded;

  static const int8_t table[16] = {
      0, -1, 1, 0,
      1, 0, 0, -1,
      -1, 0, 0, 1,
      0, 1, -1, 0};

  encoderAccum += table[sum];
  lastEncoded = encoded;
}

// --- Networking ---

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to Wi-Fi \"%s\"", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.printf("\nConnected, IP: %s\n", WiFi.localIP().toString().c_str());
}

// Sends a JSON body (already serialized) to a single node's /json/state
// endpoint. Returns true on a successful (2xx) response.
bool postState(const char* host, const String& body) {
  WiFiClient client;
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);

  String url = String("http://") + host + "/json/state";
  if (!http.begin(client, url)) {
    Serial.printf("  %s: begin() failed\n", host);
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  http.end();

  if (code <= 0) {
    Serial.printf("  %s: request failed (%s)\n", host, HTTPClient::errorToString(code).c_str());
    return false;
  }
  if (code >= 300) {
    Serial.printf("  %s: HTTP %d\n", host, code);
    return false;
  }
  return true;
}

// Sends the same JSON body to every configured node.
void broadcastState(const String& body) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi not connected, skipping broadcast");
    return;
  }
  for (size_t i = 0; i < WLED_NODE_COUNT; i++) {
    postState(WLED_NODES[i], body);
  }
}

// --- Mode / brightness -> WLED JSON state ---

String buildModeStateJson(Mode mode, uint8_t bri) {
  JsonDocument doc;

  if (mode == MODE_OFF) {
    doc["on"] = false;
    doc["bri"] = bri;
  } else {
    doc["on"] = true;
    doc["bri"] = bri;
    JsonObject seg = doc["seg"].add<JsonObject>();
    seg["fx"] = 0;  // solid color effect
    JsonArray col0 = seg["col"].add<JsonArray>();
    if (mode == MODE_RED) {
      col0.add(255);
      col0.add(0);
      col0.add(0);
    } else {  // MODE_WHITE
      col0.add(255);
      col0.add(255);
      col0.add(255);
    }
  }

  String out;
  serializeJson(doc, out);
  return out;
}

String buildBrightnessStateJson(uint8_t bri) {
  JsonDocument doc;
  doc["bri"] = bri;
  String out;
  serializeJson(doc, out);
  return out;
}

void applyMode(Mode mode) {
  currentMode = mode;
  const char* names[MODE_COUNT] = {"RED", "WHITE", "OFF"};
  Serial.printf("Mode -> %s (bri=%u)\n", names[mode], brightness);
  broadcastState(buildModeStateJson(mode, brightness));
}

void applyBrightness(uint8_t bri) {
  brightness = bri;
  Serial.printf("Brightness -> %u\n", brightness);
  broadcastState(buildBrightnessStateJson(bri));
}

// --- Button handling (polled, debounced) ---

void handleButton() {
  static bool lastReading = HIGH;
  static bool stableState = HIGH;
  static unsigned long lastChangeMs = 0;

  bool reading = digitalRead(PIN_ENCODER_BTN);
  if (reading != lastReading) {
    lastChangeMs = millis();
    lastReading = reading;
  }

  if ((millis() - lastChangeMs) > BUTTON_DEBOUNCE_MS && reading != stableState) {
    stableState = reading;
    if (stableState == LOW) {  // active-low button press
      Mode next = static_cast<Mode>((currentMode + 1) % MODE_COUNT);
      applyMode(next);
    }
  }
}

// --- Encoder handling (polled accumulator, throttled sends) ---

void handleEncoder() {
  static unsigned long lastSendMs = 0;
  static bool pendingSend = false;

  noInterrupts();
  int32_t accum = encoderAccum;
  interrupts();

  int32_t steps = accum / ENCODER_PULSES_PER_STEP;
  if (steps != 0) {
    noInterrupts();
    encoderAccum -= steps * ENCODER_PULSES_PER_STEP;
    interrupts();

    int32_t newBrightness = static_cast<int32_t>(brightness) + steps * BRIGHTNESS_STEP;
    newBrightness = constrain(newBrightness, BRIGHTNESS_MIN, BRIGHTNESS_MAX);

    if (static_cast<uint8_t>(newBrightness) != brightness) {
      brightness = static_cast<uint8_t>(newBrightness);
      pendingSend = true;
    }
  }

  // Throttle network sends so a fast spin doesn't flood every node with a
  // request per detent; the latest value is flushed once the interval opens.
  if (pendingSend && millis() - lastSendMs >= ENCODER_SEND_INTERVAL_MS) {
    lastSendMs = millis();
    pendingSend = false;
    applyBrightness(brightness);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_ENCODER_A, INPUT_PULLUP);
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);
  pinMode(PIN_ENCODER_BTN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_B), encoderISR, CHANGE);

  connectWiFi();

  // Push the default (boot) state to every node.
  applyMode(currentMode);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  handleButton();
  handleEncoder();

  delay(5);
}
