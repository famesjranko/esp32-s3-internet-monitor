// Host preview: serves the firmware's real web handlers on localhost.
//   ./preview [--port N] [--portal]
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

// Firmware headers (found through -I$(FW_DIR)), in the order the sketch needs them. effects_base.h gives the real effect names and defaults.
#include "effects/effects_base.h"
#include "core/crypto.h"
#include "web/server.h"
#include "web/portal.h"

#include "fake_state.h"

namespace {

std::string urlDecode(const std::string& s) {
  std::string out;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '+') out += ' ';
    else if (s[i] == '%' && i + 2 < s.size()) { out += (char)std::strtol(s.substr(i + 1, 2).c_str(), nullptr, 16); i += 2; }
    else out += s[i];
  }
  return out;
}

void parseForm(const std::string& s, std::map<std::string, std::string>& out) {
  size_t pos = 0;
  while (pos <= s.size() && !s.empty()) {
    size_t amp = s.find('&', pos);
    std::string pair = s.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
    size_t eq = pair.find('=');
    if (!pair.empty()) out[urlDecode(pair.substr(0, eq))] = eq == std::string::npos ? "" : urlDecode(pair.substr(eq + 1));
    if (amp == std::string::npos) break;
    pos = amp + 1;
  }
}

std::string readRequest(int fd) {
  std::string data;
  char buf[4096];
  size_t bodyAt = std::string::npos, want = 0;
  while (true) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n <= 0) break;
    data.append(buf, (size_t)n);
    if (bodyAt == std::string::npos) {
      size_t end = data.find("\r\n\r\n");
      if (end == std::string::npos) continue;
      bodyAt = end + 4;
      std::string head = data.substr(0, end);
      for (auto& c : head) c = (char)std::tolower((unsigned char)c);
      size_t cl = head.find("content-length:");
      if (cl != std::string::npos) want = (size_t)std::strtoul(head.c_str() + cl + 15, nullptr, 10);
    }
    if (data.size() >= bodyAt + want) break;
  }
  return data;
}

void handleConnection(int fd) {
  std::string raw = readRequest(fd);
  size_t lineEnd = raw.find("\r\n");
  size_t headEnd = raw.find("\r\n\r\n");
  if (lineEnd == std::string::npos || headEnd == std::string::npos) return;

  std::string line = raw.substr(0, lineEnd);
  size_t s1 = line.find(' '), s2 = line.find(' ', s1 + 1);
  std::string verb = line.substr(0, s1), target = line.substr(s1 + 1, s2 - s1 - 1);

  std::map<std::string, std::string> args, headers;
  size_t q = target.find('?');
  if (q != std::string::npos) parseForm(target.substr(q + 1), args);
  parseForm(raw.substr(headEnd + 4), args);

  size_t pos = lineEnd + 2;
  while (pos < headEnd) {
    size_t e = raw.find("\r\n", pos);
    std::string h = raw.substr(pos, e - pos);
    size_t colon = h.find(':');
    if (colon != std::string::npos) {
      size_t v = h.find_first_not_of(' ', colon + 1);
      headers[h.substr(0, colon)] = v == std::string::npos ? "" : h.substr(v);
    }
    pos = e + 2;
  }

  HTTPMethod m = verb == "POST" ? HTTP_POST : HTTP_GET;
  auto resp = server.dispatch(m, target.substr(0, q), args, headers);
  std::printf("%s %s -> %d\n", verb.c_str(), target.c_str(), resp.status);

  std::string out = "HTTP/1.1 " + std::to_string(resp.status) + " OK\r\nContent-Type: " + resp.contentType +
                    "; charset=utf-8\r\nContent-Length: " + std::to_string(resp.body.size()) + "\r\nConnection: close\r\n";
  for (auto& h : resp.headers) out += h.first + ": " + h.second + "\r\n";
  out += "\r\n" + resp.body;
  for (size_t sent = 0; sent < out.size();) {
    ssize_t n = write(fd, out.data() + sent, out.size() - sent);
    if (n <= 0) break;
    sent += (size_t)n;
  }
}

int listenLocal(int port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  int on = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // never expose the preview beyond this machine
  addr.sin_port = htons((uint16_t)port);
  if (bind(fd, (sockaddr*)&addr, sizeof(addr)) != 0 || listen(fd, 16) != 0) {
    std::perror("bind/listen");
    std::exit(1);
  }
  return fd;
}

}  // namespace

int main(int argc, char** argv) {
  int port = 8080;
  bool portal = false;
  for (int i = 1; i < argc; i++) {
    if (std::strcmp(argv[i], "--portal") == 0) portal = true;
    else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = std::atoi(argv[++i]);
    else { std::fprintf(stderr, "usage: %s [--port N] [--portal]\n", argv[0]); return 2; }
  }

  initFakeState();
  if (portal) {
    currentState = STATE_CONFIG_PORTAL;
    enterConfigMode();  // the firmware's own portal start-up: scan, build list, register routes
  } else {
    setupWebServer();
  }

  int lfd = listenLocal(port);
  std::printf("preview (%s) on http://127.0.0.1:%d/\n", portal ? "portal" : "dashboard", port);
  std::fflush(stdout);
  while (true) {
    int cfd = accept(lfd, nullptr, nullptr);
    if (cfd < 0) continue;
    handleConnection(cfd);
    close(cfd);
  }
}
