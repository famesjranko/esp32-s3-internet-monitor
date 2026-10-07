// Fake device state. Included once by preview.cpp (the firmware headers define globals, so one translation unit).
#pragma once

WebServer server;
DNSServer dnsServer;
Adafruit_NeoPixel pixels;
Preferences preferences;
portMUX_TYPE stateMux = 0;

SystemStats stats;
PerformanceMetrics perf;
AuthState auth;

volatile int currentState = STATE_INTERNET_OK;
volatile int currentEffect = EFFECT_RAIN;
volatile uint8_t currentBrightness = 18;
volatile uint8_t currentRotation = DEFAULT_ROTATION;
volatile uint8_t effectSpeed = 36;

// LED colour fade state that effects_base.h and changeState() read; the preview never renders LEDs.
volatile uint8_t currentR = 0, currentG = 0, currentB = 0;
volatile uint8_t targetR = 0, targetG = 0, targetB = 0;
uint8_t fadeStartR = 0, fadeStartG = 0, fadeStartB = 0;
unsigned long fadeStartTime = 0;
unsigned long stateChangeTime = 0;
volatile bool isInternetOK = true;

// Per-effect resets that resetAllEffectState() calls; they only clear animation buffers.
void resetBallEffect() {}
void resetLifeEffect() {}
void resetMatrixEffect() {}
void resetNoiseEffect() {}
void resetPongEffect() {}
void resetRainEffect() {}

bool configPortalActive = false;
unsigned long lastPortalActivity = 0;
String cachedNetworkListHTML;

String storedSSID = "HomeNetwork";
String storedPassword;
String storedWebPasswordHash;
bool settingsPendingSave = false;
unsigned long lastSettingChangeTime = 0;

MQTTConfig mqttConfig;

// About three days up, one short outage, MQTT connected with Home Assistant discovery off.
inline void initFakeState() {
  g_millisOffset = (3UL * 24 + 4) * 3600000UL + 17UL * 60000UL;
  stats.bootTime = 0;
  stats.totalChecks = 26104;
  stats.failedChecks = 52;
  stats.successfulChecks = stats.totalChecks - stats.failedChecks;
  stats.lastDowntime = 160000;
  stats.totalDowntimeMs = 320000;

  perf.ledActualFPS = 59.8f;
  perf.ledFrameTimeUs = 4120;
  perf.ledMaxFrameTimeUs = 9870;
  perf.ledStackHighWater = 1340;
  perf.netStackHighWater = 2210;

  storedWebPasswordHash = sha256("admin");  // the firmware default password

  mqttConfig.enabled = true;
  strncpy(mqttConfig.broker, "homeassistant.local", sizeof(mqttConfig.broker) - 1);
  strncpy(mqttConfig.username, "monitor", sizeof(mqttConfig.username) - 1);
  strncpy(mqttConfig.password, "preview-secret", sizeof(mqttConfig.password) - 1);
  mqttConfig.homeAssistantDiscovery = false;
  mqttConfig.connected = true;
}
