// The preview never opens a broker connection; every call is a no-op.
#pragma once
#include "WiFi.h"
typedef void (*MQTT_CALLBACK_SIGNATURE)(char*, byte*, unsigned int);
class PubSubClient {
 public:
  explicit PubSubClient(WiFiClient&) {}
  PubSubClient& setServer(const char*, uint16_t) { return *this; }
  PubSubClient& setCallback(MQTT_CALLBACK_SIGNATURE) { return *this; }
  PubSubClient& setKeepAlive(uint16_t) { return *this; }
  PubSubClient& setSocketTimeout(uint16_t) { return *this; }
  bool setBufferSize(uint16_t) { return true; }
  bool connected() { return false; }
  bool connect(...) { return false; }
  void disconnect() {}
  bool loop() { return false; }
  int state() { return -1; }
  bool publish(...) { return false; }
};
