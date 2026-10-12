// Fixed fake network identity. The scan list uses invented SSIDs only.
#pragma once
#include "Arduino.h"

enum wl_status_t { WL_IDLE_STATUS = 0, WL_CONNECTED = 3, WL_DISCONNECTED = 6 };
enum wifi_auth_mode_t { WIFI_AUTH_OPEN = 0, WIFI_AUTH_WPA2_PSK = 3 };
enum WiFiMode_t { WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3 };
constexpr int WIFI_SCAN_RUNNING = -1;
constexpr int WIFI_SCAN_FAILED = -2;

struct IPAddress {
  uint8_t a, b, c, d;
  IPAddress(uint8_t a_ = 0, uint8_t b_ = 0, uint8_t c_ = 0, uint8_t d_ = 0) : a(a_), b(b_), c(c_), d(d_) {}
  String toString() const { return String((int)a) + "." + String((int)b) + "." + String((int)c) + "." + String((int)d); }
};

struct WiFiClient {};

class WiFiClass {
 public:
  String SSID() const { return String("HomeNetwork"); }
  String SSID(int i) const { return String(kNets[i].ssid); }
  int RSSI(int i) const { return kNets[i].rssi; }
  int RSSI() const { return -52; }
  wifi_auth_mode_t encryptionType(int i) const { return kNets[i].open ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK; }
  IPAddress localIP() const { return IPAddress(192, 168, 1, 50); }
  IPAddress softAPIP() const { return IPAddress(192, 168, 4, 1); }
  String macAddress() const { return String("02:00:00:00:00:01"); }  // locally administered, not a real device
  int channel() const { return 6; }
  wl_status_t status() const { return WL_CONNECTED; }
  bool mode(WiFiMode_t m) { mode_ = m; return true; }
  WiFiMode_t getMode() const { return mode_; }
  void disconnect(bool = false, bool = false) {}
  void begin(const char*, const char*) {}
  bool softAP(const char*, const char* = nullptr, int = 1) { return true; }
  // A blocking scan returns the list at once. An async scan reports "done" on the next scanComplete().
  int scanNetworks(bool async = false, bool = false, bool = false, unsigned = 300) {
    done_ = true;
    return async ? WIFI_SCAN_RUNNING : kCount;
  }
  int scanComplete() const { return done_ ? kCount : WIFI_SCAN_FAILED; }
  void scanDelete() { done_ = false; }

 private:
  struct Net { const char* ssid; int rssi; bool open; };
  static constexpr int kCount = 5;
  static constexpr Net kNets[kCount] = {
      {"HomeNetwork", -52, false}, {"Neighbour-2G", -61, false}, {"CafeGuest", -68, true},
      {"OfficeWiFi", -74, false}, {"IoT-Lab", -83, false}};
  bool done_ = false;
  WiFiMode_t mode_ = WIFI_AP;
};
inline WiFiClass WiFi;
