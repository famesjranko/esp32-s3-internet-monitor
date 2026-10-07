// In-memory key/value store standing in for NVS.
#pragma once
#include <map>
#include "Arduino.h"
class Preferences {
 public:
  bool begin(const char*, bool = false) { return true; }
  void end() {}
  void clear() { kv_.clear(); }
  bool isKey(const char* k) { return kv_.count(k) > 0; }
  void remove(const char* k) { kv_.erase(k); }
  bool getBool(const char* k, bool d = false) { return kv_.count(k) ? kv_[k] == "1" : d; }
  uint8_t getUChar(const char* k, uint8_t d = 0) { return (uint8_t)num(k, d); }
  uint16_t getUShort(const char* k, uint16_t d = 0) { return (uint16_t)num(k, d); }
  uint32_t getULong(const char* k, uint32_t d = 0) { return (uint32_t)num(k, d); }
  String getString(const char* k, const String& d = String()) { return kv_.count(k) ? String(kv_[k]) : d; }
  void putBool(const char* k, bool v) { kv_[k] = v ? "1" : "0"; }
  void putUChar(const char* k, uint8_t v) { kv_[k] = std::to_string(v); }
  void putUShort(const char* k, uint16_t v) { kv_[k] = std::to_string(v); }
  void putULong(const char* k, uint32_t v) { kv_[k] = std::to_string(v); }
  void putString(const char* k, const String& v) { kv_[k] = v.c_str(); }
 private:
  unsigned long num(const char* k, unsigned long d) { return kv_.count(k) ? std::strtoul(kv_[k].c_str(), nullptr, 10) : d; }
  std::map<std::string, std::string> kv_;
};
