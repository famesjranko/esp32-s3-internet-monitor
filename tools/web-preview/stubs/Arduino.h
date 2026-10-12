// Host stand-in for the Arduino core: only what the web, mqtt and storage code calls.
#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>

using std::abs;
using std::fabs;
using std::fmod;
using std::max;
using std::min;
using std::pow;
using std::sin;
using std::cos;
using std::sqrt;
constexpr double PI = 3.14159265358979323846;

#define PROGMEM
#define F(x) (x)

typedef uint8_t byte;

class String {
 public:
  String() = default;
  String(const char* s) : s_(s ? s : "") {}
  String(char c) : s_(1, c) {}
  String(const std::string& s) : s_(s) {}
  template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
  String(T v) : s_(std::to_string(v)) {}
  String(float v, int decimals = 2) { fmt(v, decimals); }
  String(double v, int decimals = 2) { fmt(v, decimals); }

  const char* c_str() const { return s_.c_str(); }
  size_t length() const { return s_.size(); }
  char charAt(size_t i) const { return i < s_.size() ? s_[i] : 0; }
  void reserve(size_t n) { s_.reserve(n); }
  bool concat(const char* c) { s_ += c; return true; }
  bool concat(const String& o) { s_ += o.s_; return true; }
  bool concat(char c) { s_ += c; return true; }
  int indexOf(char c) const { auto p = s_.find(c); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(const String& o) const { auto p = s_.find(o.s_); return p == std::string::npos ? -1 : (int)p; }
  String substring(size_t from) const { return from >= s_.size() ? String() : String(s_.substr(from)); }
  String substring(size_t from, size_t to) const {
    if (from >= s_.size() || to <= from) return String();
    return String(s_.substr(from, to - from));
  }
  void remove(size_t idx) { if (idx < s_.size()) s_.erase(idx); }
  void remove(size_t idx, size_t count) { if (idx < s_.size()) s_.erase(idx, count); }
  void replace(const String& from, const String& to) {
    if (from.s_.empty()) return;
    for (size_t p = 0; (p = s_.find(from.s_, p)) != std::string::npos; p += to.s_.size()) s_.replace(p, from.s_.size(), to.s_);
  }
  long toInt() const { return std::strtol(s_.c_str(), nullptr, 10); }
  void toLowerCase() { std::transform(s_.begin(), s_.end(), s_.begin(), [](unsigned char c) { return std::tolower(c); }); }
  void trim() {
    size_t b = s_.find_first_not_of(" \t\r\n");
    size_t e = s_.find_last_not_of(" \t\r\n");
    s_ = b == std::string::npos ? "" : s_.substr(b, e - b + 1);
  }

  String& operator+=(const String& o) { s_ += o.s_; return *this; }
  String& operator+=(const char* o) { s_ += o; return *this; }
  String& operator+=(char c) { s_ += c; return *this; }
  bool operator==(const String& o) const { return s_ == o.s_; }
  bool operator==(const char* o) const { return s_ == o; }
  bool operator!=(const String& o) const { return s_ != o.s_; }
  bool operator<(const String& o) const { return s_ < o.s_; }
  char operator[](size_t i) const { return charAt(i); }
  friend String operator+(const String& a, const String& b) { return String(a.s_ + b.s_); }
  friend String operator+(const char* a, const String& b) { return String(std::string(a) + b.s_); }
  friend String operator+(const String& a, const char* b) { return String(a.s_ + b); }

 private:
  template <typename F>
  void fmt(F v, int decimals) {
    char buf[48];
    snprintf(buf, sizeof(buf), "%.*f", decimals, (double)v);
    s_ = buf;
  }
  std::string s_;
};

// Reading a data block through FPSTR is a plain pointer on the host.
#define FPSTR(p) (p)

// The preview clock starts at a fake uptime so the dashboard shows days of uptime.
inline unsigned long g_millisOffset = 0;
inline unsigned long millis() {
  using namespace std::chrono;
  static const auto start = steady_clock::now();
  return g_millisOffset + (unsigned long)duration_cast<milliseconds>(steady_clock::now() - start).count();
}
inline void delay(unsigned long) {}
inline uint32_t esp_random() { return (uint32_t)std::rand(); }
inline float temperatureRead() { return 48.3f; }

// BSD strlcpy, which the ESP32 core provides.
inline size_t strlcpy(char* dst, const char* src, size_t n) {
  size_t len = std::strlen(src);
  if (n) { size_t c = len >= n ? n - 1 : len; std::memcpy(dst, src, c); dst[c] = 0; }
  return len;
}

struct SerialStub {
  template <typename T> void print(const T&) {}
  template <typename T> void println(const T&) {}
  void println() {}
  template <typename... A> void printf(const char*, A...) {}
};
inline SerialStub Serial;

struct EspStub {
  unsigned getCpuFreqMHz() { return 240; }
  unsigned getFreeHeap() { return 187 * 1024; }
  unsigned getMinFreeHeap() { return 142 * 1024; }
  unsigned getFlashChipSize() { return 8 * 1024 * 1024; }
  unsigned getSketchSize() { return 1096 * 1024; }
  void restart() { std::puts("[preview] ESP.restart() ignored"); }
};
inline EspStub ESP;
