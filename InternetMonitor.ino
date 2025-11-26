/*
 * ESP32-S3 Internet Monitor - Full Featured
 * 
 * Features:
 * - Multiple fallback check URLs
 * - Consecutive failure threshold (avoids false alarms)
 * - Watchdog timer (auto-reboot on hang)
 * - Smooth fade transitions
 * - Heartbeat pulse animation
 * - Web interface with stats
 * - OTA updates (over-the-air)
 * - Uptime/downtime tracking
 * - Non-blocking design
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <esp_task_wdt.h>

// ===========================================
// CONFIGURATION
// ===========================================

// WiFi
const char* WIFI_SSID     = "";
const char* WIFI_PASSWORD = "";

// Hardware
#define RGB_PIN           14
#define NUM_LEDS          64
#define LED_BRIGHTNESS    30    // Max 50! Higher can damage board

// Timing (milliseconds)
#define CHECK_INTERVAL    10000  // Check internet every 10 seconds
#define HTTP_TIMEOUT      5000   // 5 second timeout per request
#define WIFI_TIMEOUT      20000  // 20 seconds to connect
#define HEARTBEAT_INTERVAL 2000  // Pulse every 2 seconds
#define FADE_DURATION     500    // 500ms fade transitions
#define WDT_TIMEOUT       60     // Watchdog: reboot if stuck for 60 sec

// Failure threshold
#define FAILURES_BEFORE_RED  2   // Need 2 consecutive failures to show red

// ===========================================
// STATE MACHINE
// ===========================================

enum State {
  STATE_BOOTING,
  STATE_CONNECTING_WIFI,
  STATE_WIFI_LOST,
  STATE_INTERNET_OK,
  STATE_INTERNET_DEGRADED,  // Some checks failing
  STATE_INTERNET_DOWN
};

State currentState = STATE_BOOTING;

// ===========================================
// LED EFFECTS
// ===========================================

enum Effect {
  EFFECT_OFF,
  EFFECT_SOLID,
  EFFECT_RIPPLE,
  EFFECT_RAINBOW,
  EFFECT_PULSE,
  EFFECT_RAIN
};

Effect currentEffect = EFFECT_RAIN;  // Default effect
const char* effectNames[] = {"Off", "Solid", "Ripple", "Rainbow", "Pulse", "Rain"};
uint8_t currentBrightness = 21;  // Default brightness (max 50 for safety)
uint8_t currentRotation = 2;     // 0=0°, 1=90°, 2=180°, 3=270°
uint8_t effectSpeed = 80;        // 1-100, default 80

// ===========================================
// GLOBALS
// ===========================================

Adafruit_NeoPixel pixels(NUM_LEDS, RGB_PIN, NEO_RGB + NEO_KHZ800);
WebServer server(80);

// Timing
unsigned long lastCheck = 0;
unsigned long lastHeartbeat = 0;
unsigned long stateChangeTime = 0;
unsigned long bootTime = 0;

// Stats
unsigned long totalChecks = 0;
unsigned long successfulChecks = 0;
unsigned long failedChecks = 0;
int consecutiveFailures = 0;
int consecutiveSuccesses = 0;
unsigned long lastDowntime = 0;
unsigned long totalDowntimeMs = 0;
unsigned long downtimeStart = 0;
bool wasDown = false;

// Current and target colors for fading
uint8_t currentR = 0, currentG = 0, currentB = 0;
uint8_t targetR = 0, targetG = 0, targetB = 0;
unsigned long fadeStartTime = 0;
uint8_t fadeStartR = 0, fadeStartG = 0, fadeStartB = 0;

// Heartbeat is now time-based, no phase variable needed

// Check URLs (multiple for redundancy)
const char* checkUrls[] = {
  "http://clients3.google.com/generate_204",
  "http://www.gstatic.com/generate_204",
  "http://connectivitycheck.gstatic.com/generate_204",
  "http://cp.cloudflare.com/"  // Returns 204
};
const int numCheckUrls = 4;

// ===========================================
// LED FUNCTIONS
// ===========================================

// Map logical row/col to physical pixel index based on rotation
int getPixelIndex(int row, int col) {
  int r = row, c = col;
  switch (currentRotation) {
    case 1:  // 90° CW
      r = col;
      c = 7 - row;
      break;
    case 2:  // 180°
      r = 7 - row;
      c = 7 - col;
      break;
    case 3:  // 270° CW
      r = 7 - col;
      c = row;
      break;
  }
  return r * 8 + c;
}

void setTargetColor(uint8_t r, uint8_t g, uint8_t b) {
  if (targetR == r && targetG == g && targetB == b) return;
  
  fadeStartR = currentR;
  fadeStartG = currentG;
  fadeStartB = currentB;
  targetR = r;
  targetG = g;
  targetB = b;
  fadeStartTime = millis();
}

void updateFade() {
  unsigned long elapsed = millis() - fadeStartTime;
  
  if (elapsed >= FADE_DURATION) {
    currentR = targetR;
    currentG = targetG;
    currentB = targetB;
  } else {
    float progress = (float)elapsed / FADE_DURATION;
    // Ease in-out
    progress = progress < 0.5 ? 2 * progress * progress : 1 - pow(-2 * progress + 2, 2) / 2;
    
    currentR = fadeStartR + (targetR - fadeStartR) * progress;
    currentG = fadeStartG + (targetG - fadeStartG) * progress;
    currentB = fadeStartB + (targetB - fadeStartB) * progress;
  }
}

// --- Effect: Solid color (no animation) ---
void effectSolid() {
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, pixels.Color(currentR, currentG, currentB));
  }
  pixels.show();
}

// --- Effect: Diagonal ripple wave ---
void effectRipple() {
  unsigned long now = millis();
  float speedMult = effectSpeed / 50.0;  // 0.02 to 2.0
  float timeScale = now / 1000.0 * speedMult;
  
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      int i = getPixelIndex(row, col);
      float distance = (row + col) / 14.0;
      float wave = sin((distance * 4.0) - (timeScale * 2.0));
      wave = (wave + 1.0) / 2.0;
      
      // Accent color based on state
      uint8_t r2, g2, b2;
      if (currentG > currentR && currentG > currentB) {
        r2 = 0; g2 = currentG * 0.6; b2 = currentG * 0.6;  // Cyan
      } else if (currentR > currentG && currentR > currentB) {
        r2 = currentR; g2 = currentR * 0.3; b2 = 0;  // Orange
      } else if (currentB > currentR && currentB > currentG) {
        r2 = currentB * 0.4; g2 = 0; b2 = currentB;  // Purple
      } else {
        r2 = currentR; g2 = currentG * 0.4; b2 = 0;  // Amber
      }
      
      uint8_t r = currentR + (int)(r2 - currentR) * wave;
      uint8_t g = currentG + (int)(g2 - currentG) * wave;
      uint8_t b = currentB + (int)(b2 - currentB) * wave;
      
      pixels.setPixelColor(i, pixels.Color(r, g, b));
    }
  }
  pixels.show();
}

// --- Effect: Rainbow wave (state color tints the rainbow) ---
void effectRainbow() {
  unsigned long now = millis();
  float speedMult = effectSpeed / 50.0;
  
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      int i = getPixelIndex(row, col);
      
      // Rainbow hue based on position and time
      uint16_t hue = (row + col) * 4096 + (uint32_t)(now * 20 * speedMult);
      uint32_t rainbow = pixels.ColorHSV(hue, 255, 60);
      
      // Extract RGB from rainbow
      uint8_t rb = (rainbow >> 16) & 0xFF;
      uint8_t gb = (rainbow >> 8) & 0xFF;
      uint8_t bb = rainbow & 0xFF;
      
      // Blend with state color (70% rainbow, 30% state)
      uint8_t r = (rb * 7 + currentR * 3) / 10;
      uint8_t g = (gb * 7 + currentG * 3) / 10;
      uint8_t b = (bb * 7 + currentB * 3) / 10;
      
      pixels.setPixelColor(i, pixels.Color(r, g, b));
    }
  }
  pixels.show();
}

// --- Effect: Gentle pulse/breathe ---
void effectPulse() {
  unsigned long now = millis();
  float speedMult = effectSpeed / 50.0;
  float cycleTime = 3000.0 / speedMult;  // Faster speed = shorter cycle
  float phase = fmod(now, cycleTime) / cycleTime;
  float breath = (sin(phase * 6.2832) + 1.0) / 2.0;
  breath = 0.4 + 0.6 * breath;
  
  uint8_t r = currentR * breath;
  uint8_t g = currentG * breath;
  uint8_t b = currentB * breath;
  
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
}

// --- Effect: Rain drops falling ---
void effectRain() {
  static uint8_t drops[8] = {0, 2, 5, 1, 7, 3, 6, 4};  // Drop positions per column
  static unsigned long lastUpdate = 0;
  
  unsigned long now = millis();
  float speedMult = effectSpeed / 50.0;
  int updateInterval = 150 / speedMult;  // Faster speed = shorter interval
  if (updateInterval < 30) updateInterval = 30;  // Minimum 30ms
  
  // Update drop positions
  if (now - lastUpdate > updateInterval) {
    lastUpdate = now;
    for (int col = 0; col < 8; col++) {
      drops[col] = (drops[col] + 1) % 12;  // 12 steps: 8 visible + 4 gap
    }
  }
  
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      int i = getPixelIndex(row, col);
      
      int dropRow = drops[col];
      int dist = abs(row - dropRow);
      
      if (dropRow < 8 && dist <= 2) {
        // Near the drop - bright
        float bright = 1.0 - (dist * 0.35);
        pixels.setPixelColor(i, pixels.Color(
          currentR * bright,
          currentG * bright,
          currentB * bright
        ));
      } else {
        // Background - dim
        pixels.setPixelColor(i, pixels.Color(
          currentR * 0.15,
          currentG * 0.15,
          currentB * 0.15
        ));
      }
    }
  }
  pixels.show();
}

// --- Effect: Off ---
void effectOff() {
  pixels.clear();
  pixels.show();
}

// --- Apply current effect ---
void applyEffect() {
  switch (currentEffect) {
    case EFFECT_OFF:     effectOff(); break;
    case EFFECT_SOLID:   effectSolid(); break;
    case EFFECT_RIPPLE:  effectRipple(); break;
    case EFFECT_RAINBOW: effectRainbow(); break;
    case EFFECT_PULSE:   effectPulse(); break;
    case EFFECT_RAIN:    effectRain(); break;
    default:             effectSolid(); break;
  }
}

void fillMatrixImmediate(uint8_t r, uint8_t g, uint8_t b) {
  currentR = targetR = r;
  currentG = targetG = g;
  currentB = targetB = b;
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
}

// ===========================================
// INTERNET CHECK
// ===========================================

bool checkSingleUrl(const char* url) {
  HTTPClient http;
  http.setConnectTimeout(2000);  // 2 sec connect timeout
  http.setTimeout(3000);          // 3 sec response timeout
  
  if (!http.begin(url)) {
    return false;
  }
  
  int code = http.GET();
  http.end();
  return (code == 204 || code == 200);
}

// Returns number of successful checks (0 to numCheckUrls)
int checkInternet() {
  int successes = 0;
  
  // Only try 2 URLs max to keep check fast
  for (int i = 0; i < 2; i++) {
    esp_task_wdt_reset();  // Pet the watchdog before each request
    
    if (checkSingleUrl(checkUrls[i])) {
      successes++;
      if (successes >= 1) {
        // One success is enough to confirm internet is up
        esp_task_wdt_reset();
        return successes;
      }
    }
    
    esp_task_wdt_reset();  // Pet after each request too
  }
  
  return successes;
}

// ===========================================
// STATE MANAGEMENT
// ===========================================

void changeState(State newState) {
  if (currentState == newState) return;
  
  Serial.print("State: ");
  Serial.print(currentState);
  Serial.print(" -> ");
  Serial.println(newState);
  
  // Track downtime
  if (newState == STATE_INTERNET_DOWN && !wasDown) {
    downtimeStart = millis();
    wasDown = true;
  } else if (newState == STATE_INTERNET_OK && wasDown) {
    totalDowntimeMs += millis() - downtimeStart;
    lastDowntime = millis() - downtimeStart;
    wasDown = false;
  }
  
  currentState = newState;
  stateChangeTime = millis();
  
  // Set colors based on state
  switch (newState) {
    case STATE_BOOTING:
      setTargetColor(0, 0, 80);       // Blue
      break;
    case STATE_CONNECTING_WIFI:
      setTargetColor(0, 40, 80);      // Cyan
      break;
    case STATE_WIFI_LOST:
      setTargetColor(100, 0, 0);       // Red
      break;
    case STATE_INTERNET_OK:
      setTargetColor(0, 80, 0);       // Green
      break;
    case STATE_INTERNET_DEGRADED:
      setTargetColor(80, 60, 0);      // Yellow
      break;
    case STATE_INTERNET_DOWN:
      setTargetColor(100, 20, 0);      // Orange-Red
      break;
  }
}

// ===========================================
// WEB SERVER
// ===========================================

String formatUptime(unsigned long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;
  
  String result = "";
  if (days > 0) result += String(days) + "d ";
  if (hours % 24 > 0) result += String(hours % 24) + "h ";
  if (minutes % 60 > 0) result += String(minutes % 60) + "m ";
  result += String(seconds % 60) + "s";
  return result;
}

void handleRoot() {
  unsigned long uptime = millis() - bootTime;
  float successRate = totalChecks > 0 ? (100.0 * successfulChecks / totalChecks) : 0;
  
  String stateStr;
  String stateColor;
  switch (currentState) {
    case STATE_INTERNET_OK: stateStr = "ONLINE"; stateColor = "#00ff00"; break;
    case STATE_INTERNET_DEGRADED: stateStr = "DEGRADED"; stateColor = "#ffaa00"; break;
    case STATE_INTERNET_DOWN: stateStr = "OFFLINE"; stateColor = "#ff0000"; break;
    case STATE_WIFI_LOST: stateStr = "NO WIFI"; stateColor = "#ff0000"; break;
    default: stateStr = "STARTING"; stateColor = "#0088ff"; break;
  }
  
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Internet Monitor</title>
  <style>
    body { font-family: -apple-system, sans-serif; background: #1a1a2e; color: #eee; 
           padding: 20px; max-width: 600px; margin: 0 auto; }
    h1 { color: #fff; margin-bottom: 5px; }
    .status { font-size: 2.5em; font-weight: bold; padding: 20px; border-radius: 10px;
              text-align: center; margin: 20px 0; }
    .card { background: #16213e; padding: 15px; border-radius: 8px; margin: 10px 0; }
    .stat { display: flex; justify-content: space-between; padding: 8px 0; 
            border-bottom: 1px solid #333; }
    .stat:last-child { border: none; }
    .label { color: #888; }
    .value { font-weight: bold; }
    .good { color: #00ff00; }
    .bad { color: #ff4444; }
    .effects { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; margin-top: 10px; }
    .effect-btn { 
      padding: 12px; border: none; border-radius: 8px; cursor: pointer;
      font-size: 14px; font-weight: bold; transition: all 0.2s;
      background: #2a2a4e; color: #ccc;
    }
    .effect-btn:hover { background: #3a3a6e; transform: scale(1.02); }
    .effect-btn.active { background: #4a4a8e; color: #fff; box-shadow: 0 0 10px rgba(100,100,255,0.5); }
    .effect-btn.off { background: #4a2a2a; }
    .effect-btn.off.active { background: #8a4a4a; }
    .brightness { margin-top: 15px; padding-top: 15px; border-top: 1px solid #333; }
    .brightness label { color: #888; display: block; margin-bottom: 8px; }
    .brightness input[type="range"] { 
      width: 100%; height: 8px; border-radius: 4px; background: #2a2a4e;
      -webkit-appearance: none; appearance: none; cursor: pointer;
    }
    .brightness input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none; width: 20px; height: 20px; border-radius: 50%;
      background: #6a6aae; cursor: pointer;
    }
    .brightness-value { 
      text-align: center; font-size: 1.2em; font-weight: bold; 
      color: #fff; margin-top: 5px; 
    }
    .rotation { margin-top: 15px; padding-top: 15px; border-top: 1px solid #333; }
    .rotation label { color: #888; display: block; margin-bottom: 8px; }
    .rotation-btns { display: flex; gap: 10px; }
    .rot-btn {
      flex: 1; padding: 10px; border: none; border-radius: 6px; cursor: pointer;
      font-size: 14px; background: #2a2a4e; color: #ccc; transition: all 0.2s;
    }
    .rot-btn:hover { background: #3a3a6e; }
    .rot-btn.active { background: #4a4a8e; color: #fff; }
    h3 { margin-top: 0; color: #aaa; }
  </style>
</head>
<body>
  <h1>🌐 Internet Monitor</h1>
  <p style="color:#888">ESP32-S3-Matrix</p>
  
  <div class="status" style="background: )rawliteral" + stateColor + R"rawliteral(22; border: 2px solid )rawliteral" + stateColor + R"rawliteral(">
    )rawliteral" + stateStr + R"rawliteral(
  </div>
  
  <div class="card">
    <h3>✨ LED Effect</h3>
    <div class="effects">
      <button class="effect-btn off)rawliteral" + String(currentEffect == EFFECT_OFF ? " active" : "") + R"rawliteral(" onclick="setEffect(0)">Off</button>
      <button class="effect-btn)rawliteral" + String(currentEffect == EFFECT_SOLID ? " active" : "") + R"rawliteral(" onclick="setEffect(1)">Solid</button>
      <button class="effect-btn)rawliteral" + String(currentEffect == EFFECT_RIPPLE ? " active" : "") + R"rawliteral(" onclick="setEffect(2)">Ripple</button>
      <button class="effect-btn)rawliteral" + String(currentEffect == EFFECT_RAINBOW ? " active" : "") + R"rawliteral(" onclick="setEffect(3)">Rainbow</button>
      <button class="effect-btn)rawliteral" + String(currentEffect == EFFECT_PULSE ? " active" : "") + R"rawliteral(" onclick="setEffect(4)">Pulse</button>
      <button class="effect-btn)rawliteral" + String(currentEffect == EFFECT_RAIN ? " active" : "") + R"rawliteral(" onclick="setEffect(5)">Rain</button>
    </div>
    <div class="brightness">
      <label>💡 Brightness</label>
      <input type="range" id="brightness" min="5" max="50" value=")rawliteral" + String(currentBrightness) + R"rawliteral(" oninput="updateBrightness(this.value)">
      <div class="brightness-value"><span id="bright-val">)rawliteral" + String(currentBrightness) + R"rawliteral(</span> / 50</div>
    </div>
    <div class="brightness">
      <label>⚡ Effect Speed</label>
      <input type="range" id="speed" min="10" max="100" value=")rawliteral" + String(effectSpeed) + R"rawliteral(" oninput="updateSpeed(this.value)">
      <div class="brightness-value"><span id="speed-val">)rawliteral" + String(effectSpeed) + R"rawliteral(</span>%</div>
    </div>
    <div class="rotation">
      <label>🔄 Rotation</label>
      <div class="rotation-btns">
        <button class="rot-btn)rawliteral" + String(currentRotation == 0 ? " active" : "") + R"rawliteral(" onclick="setRotation(0)">0°</button>
        <button class="rot-btn)rawliteral" + String(currentRotation == 1 ? " active" : "") + R"rawliteral(" onclick="setRotation(1)">90°</button>
        <button class="rot-btn)rawliteral" + String(currentRotation == 2 ? " active" : "") + R"rawliteral(" onclick="setRotation(2)">180°</button>
        <button class="rot-btn)rawliteral" + String(currentRotation == 3 ? " active" : "") + R"rawliteral(" onclick="setRotation(3)">270°</button>
      </div>
    </div>
  </div>
  
  <div class="card">
    <h3>📊 Statistics</h3>
    <div class="stat"><span class="label">Uptime</span><span class="value">)rawliteral" + formatUptime(uptime) + R"rawliteral(</span></div>
    <div class="stat"><span class="label">Total Checks</span><span class="value">)rawliteral" + String(totalChecks) + R"rawliteral(</span></div>
    <div class="stat"><span class="label">Success Rate</span><span class="value )rawliteral" + (successRate > 95 ? "good" : "bad") + R"rawliteral(">)rawliteral" + String(successRate, 1) + R"rawliteral(%</span></div>
    <div class="stat"><span class="label">Failed Checks</span><span class="value )rawliteral" + (failedChecks > 0 ? "bad" : "good") + R"rawliteral(">)rawliteral" + String(failedChecks) + R"rawliteral(</span></div>
    <div class="stat"><span class="label">Total Downtime</span><span class="value">)rawliteral" + formatUptime(totalDowntimeMs) + R"rawliteral(</span></div>
    <div class="stat"><span class="label">Last Outage Duration</span><span class="value">)rawliteral" + (lastDowntime > 0 ? formatUptime(lastDowntime) : "None") + R"rawliteral(</span></div>
  </div>
  
  <div class="card">
    <h3>🔧 Network</h3>
    <div class="stat"><span class="label">SSID</span><span class="value">)rawliteral" + String(WIFI_SSID) + R"rawliteral(</span></div>
    <div class="stat"><span class="label">IP Address</span><span class="value">)rawliteral" + WiFi.localIP().toString() + R"rawliteral(</span></div>
    <div class="stat"><span class="label">Signal Strength</span><span class="value">)rawliteral" + String(WiFi.RSSI()) + R"rawliteral( dBm</span></div>
  </div>
  
  <p style="color:#555; font-size:0.8em; text-align:center; margin-top:20px;">
    OTA updates enabled
  </p>
  
  <script>
    function setEffect(effect) {
      fetch('/effect?e=' + effect)
        .then(() => location.reload());
    }
    function updateBrightness(val) {
      document.getElementById('bright-val').textContent = val;
      fetch('/brightness?b=' + val);
    }
    function updateSpeed(val) {
      document.getElementById('speed-val').textContent = val;
      fetch('/speed?s=' + val);
    }
    function setRotation(rot) {
      fetch('/rotation?r=' + rot)
        .then(() => location.reload());
    }
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleEffect() {
  if (server.hasArg("e")) {
    int effect = server.arg("e").toInt();
    if (effect >= 0 && effect <= 5) {
      currentEffect = (Effect)effect;
      Serial.print("Effect changed to: ");
      Serial.println(effectNames[currentEffect]);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleBrightness() {
  if (server.hasArg("b")) {
    int brightness = server.arg("b").toInt();
    if (brightness >= 5 && brightness <= 50) {
      currentBrightness = brightness;
      pixels.setBrightness(currentBrightness);
      Serial.print("Brightness changed to: ");
      Serial.println(currentBrightness);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleRotation() {
  if (server.hasArg("r")) {
    int rotation = server.arg("r").toInt();
    if (rotation >= 0 && rotation <= 3) {
      currentRotation = rotation;
      Serial.print("Rotation changed to: ");
      Serial.print(rotation * 90);
      Serial.println("°");
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("s")) {
    int speed = server.arg("s").toInt();
    if (speed >= 10 && speed <= 100) {
      effectSpeed = speed;
      Serial.print("Effect speed changed to: ");
      Serial.print(effectSpeed);
      Serial.println("%");
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleStats() {
  // JSON endpoint for programmatic access
  String json = "{";
  json += "\"state\":\"" + String(currentState) + "\",";
  json += "\"effect\":\"" + String(effectNames[currentEffect]) + "\",";
  json += "\"brightness\":" + String(currentBrightness) + ",";
  json += "\"speed\":" + String(effectSpeed) + ",";
  json += "\"rotation\":" + String(currentRotation * 90) + ",";
  json += "\"uptime_ms\":" + String(millis() - bootTime) + ",";
  json += "\"total_checks\":" + String(totalChecks) + ",";
  json += "\"successful_checks\":" + String(successfulChecks) + ",";
  json += "\"failed_checks\":" + String(failedChecks) + ",";
  json += "\"consecutive_failures\":" + String(consecutiveFailures) + ",";
  json += "\"total_downtime_ms\":" + String(totalDowntimeMs) + ",";
  json += "\"rssi\":" + String(WiFi.RSSI());
  json += "}";
  
  server.send(200, "application/json", json);
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/stats", handleStats);
  server.on("/effect", handleEffect);
  server.on("/brightness", handleBrightness);
  server.on("/rotation", handleRotation);
  server.on("/speed", handleSpeed);
  server.begin();
  Serial.println("Web server started");
}

// ===========================================
// OTA SETUP
// ===========================================

void setupOTA() {
  ArduinoOTA.setHostname("internet-monitor");
  
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Update starting...");
    fillMatrixImmediate(40, 0, 40);  // Purple = updating
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA Update complete!");
    fillMatrixImmediate(0, 40, 40);  // Cyan = done
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int pct = progress / (total / 100);
    // Fill LEDs proportionally to progress
    int ledsOn = (pct * NUM_LEDS) / 100;
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i < ledsOn) {
        pixels.setPixelColor(i, pixels.Color(40, 0, 40));
      } else {
        pixels.setPixelColor(i, pixels.Color(5, 0, 5));
      }
    }
    pixels.show();
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]\n", error);
    fillMatrixImmediate(50, 0, 0);  // Red = error
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

// ===========================================
// WATCHDOG
// ===========================================

void setupWatchdog() {
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);
  Serial.println("Watchdog enabled");
}

// ===========================================
// SETUP
// ===========================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== Internet Monitor v2.0 ===");
  
  bootTime = millis();
  
  // Init LEDs
  pixels.begin();
  pixels.setBrightness(currentBrightness);
  fillMatrixImmediate(0, 0, 40);  // Blue = booting
  
  changeState(STATE_CONNECTING_WIFI);
  
  // Connect WiFi
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < WIFI_TIMEOUT) {
    delay(100);
    updateFade();
    applyEffect();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    setupWebServer();
    setupOTA();
    changeState(STATE_INTERNET_OK);
  } else {
    Serial.println("\nWiFi failed!");
    changeState(STATE_WIFI_LOST);
  }
  
  // Start watchdog AFTER wifi connected
  setupWatchdog();
  
  Serial.println("Setup complete!\n");
}

// ===========================================
// MAIN LOOP
// ===========================================

void loop() {
  esp_task_wdt_reset();  // Pet the watchdog
  
  // Handle OTA
  ArduinoOTA.handle();
  
  // Handle web requests
  server.handleClient();
  
  // Update LED fade animation
  updateFade();
  applyEffect();
  
  // Check WiFi status
  if (WiFi.status() != WL_CONNECTED) {
    if (currentState != STATE_WIFI_LOST) {
      Serial.println("WiFi lost!");
      changeState(STATE_WIFI_LOST);
    }
    esp_task_wdt_reset();
    delay(10);
    return;
  }
  
  esp_task_wdt_reset();  // Pet watchdog after WiFi check
  
  // If we just recovered WiFi
  if (currentState == STATE_WIFI_LOST) {
    Serial.println("WiFi recovered");
    changeState(STATE_INTERNET_OK);
  }
  
  // Internet check on interval
  if (millis() - lastCheck >= CHECK_INTERVAL) {
    lastCheck = millis();
    totalChecks++;
    
    esp_task_wdt_reset();  // Pet watchdog before check
    Serial.print("Checking internet... ");
    int successes = checkInternet();
    
    if (successes >= 1) {
      // Internet is up
      Serial.println("OK");
      successfulChecks++;
      consecutiveFailures = 0;
      consecutiveSuccesses++;
      
      if (currentState != STATE_INTERNET_OK) {
        changeState(STATE_INTERNET_OK);
      }
    } else {
      // Failed
      Serial.print("FAIL (consecutive: ");
      Serial.print(consecutiveFailures + 1);
      Serial.println(")");
      
      failedChecks++;
      consecutiveFailures++;
      consecutiveSuccesses = 0;
      
      // Only show down state after threshold reached
      if (consecutiveFailures >= FAILURES_BEFORE_RED) {
        changeState(STATE_INTERNET_DOWN);
      } else {
        changeState(STATE_INTERNET_DEGRADED);
      }
    }
    esp_task_wdt_reset();  // Pet watchdog after check
  }
  
  delay(10);  // Small delay to prevent tight loop
}

