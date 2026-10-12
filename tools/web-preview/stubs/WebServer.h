// Records routes and responses so a host program can dispatch real HTTP requests to the firmware handlers.
#pragma once
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "Arduino.h"

enum HTTPMethod { HTTP_ANY = 0, HTTP_GET = 1, HTTP_POST = 3 };
constexpr long CONTENT_LENGTH_UNKNOWN = -1;

class WebServer {
 public:
  typedef std::function<void()> THandlerFunction;

  // --- Registration (called by setupWebServer / setupPortalWebServer) ---
  void on(const String& uri, THandlerFunction fn) { routes_.push_back({uri.c_str(), HTTP_ANY, std::move(fn)}); }
  void on(const String& uri, HTTPMethod m, THandlerFunction fn) { routes_.push_back({uri.c_str(), m, std::move(fn)}); }
  void onNotFound(THandlerFunction fn) { notFound_ = std::move(fn); }
  void collectHeaders(const char**, size_t) {}  // every request header is kept
  void begin() {}
  void stop() {}
  void close() {}

  // --- Request view used by the handlers ---
  String arg(const String& n) { auto it = args_.find(n.c_str()); return it == args_.end() ? String() : String(it->second); }
  bool hasArg(const String& n) { return args_.count(n.c_str()) > 0; }
  bool hasHeader(const String& n) { return reqHeaders_.count(lower(n.c_str())) > 0; }
  String header(const String& n) { auto it = reqHeaders_.find(lower(n.c_str())); return it == reqHeaders_.end() ? String() : String(it->second); }
  HTTPMethod method() const { return method_; }
  String uri() const { return String(uri_); }

  // --- Response building ---
  void setContentLength(long) {}
  void sendHeader(const String& n, const String& v) { respHeaders_.push_back({n.c_str(), v.c_str()}); }
  void send(int code, const char* type = "text/html", const String& body = String()) {
    status_ = code;
    contentType_ = type;
    body_ = body.c_str();
  }
  void sendContent(const String& s) { body_ += s.c_str(); }

  // --- Host side: run one request through the registered routes ---
  struct Response {
    int status;
    std::string contentType, body;
    std::vector<std::pair<std::string, std::string>> headers;
  };
  Response dispatch(HTTPMethod m, const std::string& uri, std::map<std::string, std::string> args,
                    std::map<std::string, std::string> headers) {
    method_ = m;
    uri_ = uri;
    args_ = std::move(args);
    reqHeaders_.clear();
    for (auto& h : headers) reqHeaders_[lower(h.first)] = h.second;
    status_ = 200;
    contentType_ = "text/html";
    body_.clear();
    respHeaders_.clear();
    const Route* hit = nullptr;
    for (auto& r : routes_)
      if (r.uri == uri && (r.method == HTTP_ANY || r.method == m)) hit = &r;
    if (hit) hit->fn();
    else if (notFound_) notFound_();
    else send(404, "text/plain", "Not Found");
    return {status_, contentType_, body_, respHeaders_};
  }

 private:
  struct Route { std::string uri; HTTPMethod method; THandlerFunction fn; };
  static std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
  }
  std::vector<Route> routes_;
  THandlerFunction notFound_;
  HTTPMethod method_ = HTTP_GET;
  std::string uri_, contentType_, body_;
  std::map<std::string, std::string> args_, reqHeaders_;
  std::vector<std::pair<std::string, std::string>> respHeaders_;
  int status_ = 200;
};
