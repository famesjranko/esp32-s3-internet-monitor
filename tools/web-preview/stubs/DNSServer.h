#pragma once
#include "WiFi.h"
struct DNSServer {
  bool start(unsigned short, const String&, const IPAddress&) { return true; }
};
