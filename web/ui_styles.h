#ifndef UI_STYLES_H
#define UI_STYLES_H

/**
 * @file ui_styles.h
 * @brief CSS styles and design tokens for web UI
 * 
 * Contains all CSS including design tokens (colors, spacing, typography),
 * component styles, and responsive breakpoints. Stored in PROGMEM.
 */

// ===========================================
// DESIGN TOKENS (CSS Variables)
// ===========================================
// These define the core design system.
// Use these values when adding new UI elements.

/*
COLOR PALETTE:
  --bg-base:      #0f0f1a    (darkest background)
  --bg-card:      #1a1a2e    (card/panel background)
  --bg-input:     #252540    (input/button background)
  --bg-hover:     #303050    (hover state)
  
  --border:       #252540    (subtle borders)
  --border-light: #303048    (slightly visible borders)
  
  --text-primary:   #c8c8d8  (headings, important text)
  --text-secondary: #b8b8c8  (body text)
  --text-muted:     #808098  (secondary info)
  --text-subtle:    #707088  (labels, hints)
  --text-faint:     #505068  (disabled, decorative)
  
  --accent:       #4338ca    (primary actions, active states)
  --accent-hover: #6366f1    (accent hover)
  
  --success:      #22c55e    (online, good)
  --warning:      #f59e0b    (degraded, caution)  
  --error:        #ef4444    (offline, bad)
  --danger:       #b91c1c    (destructive actions)
  --danger-bg:    #7f1d1d    (danger button background)

SPACING:
  4px, 6px, 8px, 10px, 12px, 14px, 16px, 20px, 24px, 32px

BORDER RADIUS:
  --radius-sm: 4px   (inputs, small elements)
  --radius-md: 6px   (buttons)
  --radius-lg: 8px   (cards inner elements)
  --radius-xl: 12px  (cards, panels)

FONT SIZES:
  .6rem, .65rem, .7rem, .75rem, .8rem, .95rem, 1rem, 1.4rem, 1.6rem, 2rem
*/

// ===========================================
// BASE STYLES (reset + body)
// ===========================================

const char CSS_BASE[] PROGMEM = R"rawliteral(
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,system-ui,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;background:#0f0f1a;color:#b8b8c8;padding:16px;min-height:100vh}
.wrap{max-width:420px;margin:0 auto}
)rawliteral";

// ===========================================
// TYPOGRAPHY
// ===========================================

const char CSS_TYPOGRAPHY[] PROGMEM = R"rawliteral(
h1{font-size:1.4rem;font-weight:600;margin-bottom:4px;color:#c8c8d8}
h2{font-size:1.1rem;font-weight:600;margin-bottom:8px;color:#c8c8d8}
h3{font-size:.9rem;font-weight:600;margin-bottom:6px;color:#b8b8c8}
.sub{font-size:.7rem;color:#505068;letter-spacing:1px;text-transform:uppercase}
.text-muted{color:#707088}
.text-sm{font-size:.75rem}
.text-xs{font-size:.65rem}
)rawliteral";

// ===========================================
// LAYOUT COMPONENTS
// ===========================================

const char CSS_LAYOUT[] PROGMEM = R"rawliteral(
.hdr{display:flex;justify-content:space-between;align-items:center;margin-bottom:16px}
.hdr-left{text-align:left}
.card{background:#1a1a2e;border:1px solid #252540;border-radius:12px;padding:16px;margin-bottom:12px}
.card-title{font-size:.7rem;color:#505068;text-transform:uppercase;letter-spacing:1px;margin-bottom:12px}
.card-title.collapsible{cursor:pointer;display:flex;justify-content:space-between;align-items:center;user-select:none}
.card-title.collapsible:hover{color:#808098}
.card-title .toggle{font-size:1rem;transition:transform .2s}
.card-title.collapsed .toggle{transform:rotate(-90deg)}
.card-body{overflow:hidden;transition:max-height .3s ease-out;max-height:1000px}
.card-body.collapsed{max-height:0;margin:0;padding:0}
.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
.grid-2{display:grid;grid-template-columns:repeat(2,1fr);gap:8px}
.flex{display:flex}
.flex-between{display:flex;justify-content:space-between;align-items:center}
.gap-sm{gap:6px}
.gap-md{gap:8px}
.mt-sm{margin-top:8px}
.mt-md{margin-top:12px}
.mt-lg{margin-top:16px}
)rawliteral";

// ===========================================
// BUTTONS
// ===========================================

const char CSS_BUTTONS[] PROGMEM = R"rawliteral(
.btn{padding:10px;border:1px solid #303048;border-radius:8px;background:#252540;color:#808098;font-size:.8rem;cursor:pointer;transition:all .15s}
.btn:hover{background:#303050;color:#c8c8d8}
.btn.active{background:#4338ca;border-color:#4338ca;color:#d8d8e8}
.btn.off.active{background:#b91c1c;border-color:#b91c1c}
.btn-sm{padding:6px 12px;font-size:.7rem;border-radius:6px}
.btn-danger{background:#7f1d1d;border-color:#991b1b;color:#fca5a5}
.btn-danger:hover{background:#991b1b;color:#fef2f2}
.btn-outline{background:transparent;border:1px solid #303048;color:#808098}
.btn-outline:hover{background:#252540;color:#c8c8d8}
.logout{padding:6px 12px;border:1px solid #303048;border-radius:6px;background:#252540;color:#808098;font-size:.65rem;cursor:pointer}
.logout:hover{background:#303050;color:#c8c8d8}
)rawliteral";

// ===========================================
// FORM ELEMENTS
// ===========================================

const char CSS_FORMS[] PROGMEM = R"rawliteral(
input[type=text],input[type=password],input[type=number],input[type=email]{padding:8px 10px;background:#252540;border:1px solid #303048;border-radius:6px;color:#b8b8c8;font-size:.8rem;width:100%}
input[type=text]:focus,input[type=password]:focus,input[type=number]:focus{outline:none;border-color:#4338ca}
input[type=text]::placeholder,input[type=password]::placeholder{color:#505068}
input:-webkit-autofill,input:-webkit-autofill:focus{-webkit-box-shadow:0 0 0 1000px #252540 inset!important;-webkit-text-fill-color:#b8b8c8!important;caret-color:#b8b8c8}
input[type=range]{width:100%;height:4px;border-radius:2px;background:#303048;-webkit-appearance:none;cursor:pointer}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:16px;height:16px;border-radius:50%;background:#6366f1}
.input-row{display:flex;gap:8px;align-items:center}
.input-label{font-size:.75rem;color:#707088;margin-bottom:4px}
)rawliteral";

// ===========================================
// TOGGLE SWITCH
// ===========================================

const char CSS_TOGGLE[] PROGMEM = R"rawliteral(
.tog{position:relative;display:inline-block;width:40px;height:20px;cursor:pointer}
.tog-bg{position:absolute;top:0;left:0;right:0;bottom:0;border-radius:20px;transition:background .2s}
.tog-knob{position:absolute;height:16px;width:16px;bottom:2px;background:#fff;border-radius:50%;transition:left .2s}
)rawliteral";

// ===========================================
// STAT ROWS (key-value display)
// ===========================================

const char CSS_STATS[] PROGMEM = R"rawliteral(
.stat{display:flex;justify-content:space-between;align-items:center;padding:8px 0;border-bottom:1px solid #252540;font-size:.8rem;flex-wrap:wrap;gap:4px}
.stat:last-child{border:none}
.stat-label{color:#707088}
.stat-val{font-weight:600;color:#b8b8c8}
.good{color:#22c55e}
.warn{color:#f59e0b}
.bad{color:#ef4444}
)rawliteral";

// ===========================================
// STATUS INDICATOR
// ===========================================

const char CSS_STATUS[] PROGMEM = R"rawliteral(
.status{text-align:center;padding:24px;border-radius:12px;margin-bottom:14px;background:#1a1a2e;border:1px solid #252540}
.status-dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:10px;animation:pulse 2s infinite;vertical-align:middle}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.5}}
.status-text{font-size:1.6rem;font-weight:700;letter-spacing:2px;vertical-align:middle}
)rawliteral";

// ===========================================
// EFFECT SELECTOR (specific to dashboard)
// ===========================================

const char CSS_EFFECTS[] PROGMEM = R"rawliteral(
.effect-group{margin-bottom:10px}
.effect-group-label{font-size:.6rem;color:#505068;margin-bottom:6px;text-transform:uppercase;letter-spacing:.5px}
.slider-row{margin-top:14px;padding-top:14px;border-top:1px solid #252540}
.slider-label{display:flex;justify-content:space-between;font-size:.75rem;color:#707088;margin-bottom:8px}
.slider-val{color:#b8b8c8;font-weight:600}
.rot-row{display:flex;gap:6px;margin-top:14px;padding-top:14px;border-top:1px solid #252540;align-items:center}
.rot-row span{font-size:.75rem;color:#707088;margin-right:auto}
.rot-btn{padding:6px 12px;border:1px solid #303048;border-radius:6px;background:#252540;color:#808098;font-size:.7rem;cursor:pointer}
.rot-btn.active{background:#4338ca;border-color:#4338ca;color:#d8d8e8}
)rawliteral";

// ===========================================
// FOOTER
// ===========================================

const char CSS_FOOTER[] PROGMEM = R"rawliteral(
.footer{text-align:center;padding:14px;font-size:.65rem;color:#505068}
.footer a{color:#707088;text-decoration:none}
.footer a:hover{color:#b8b8c8}
)rawliteral";

// ===========================================
// MODAL DIALOG
// ===========================================

const char CSS_MODAL[] PROGMEM = R"rawliteral(
.modal-overlay{display:none;position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,.7);z-index:1000;align-items:center;justify-content:center;padding:16px;backdrop-filter:blur(4px)}
.modal-overlay.show{display:flex}
.modal{background:#1a1a2e;border:1px solid #303048;border-radius:12px;padding:24px;max-width:360px;width:100%;box-shadow:0 8px 32px rgba(0,0,0,.5)}
.modal-title{font-size:1rem;font-weight:600;color:#c8c8d8;margin-bottom:12px}
.modal-body{font-size:.85rem;color:#b8b8c8;line-height:1.5;margin-bottom:20px;white-space:pre-line}
.modal-footer{display:flex;gap:10px;justify-content:flex-end}
.modal-btn{padding:10px 20px;border:1px solid #303048;border-radius:8px;background:#252540;color:#808098;font-size:.85rem;cursor:pointer;transition:all .15s}
.modal-btn:hover{background:#303050;color:#c8c8d8}
.modal-btn.primary{background:#4338ca;border-color:#4338ca;color:#e8e8f8}
.modal-btn.primary:hover{background:#5248d9}
.modal-btn.danger{background:#7f1d1d;border-color:#991b1b;color:#fca5a5}
.modal-btn.danger:hover{background:#991b1b;color:#fef2f2}
)rawliteral";

// ===========================================
// RESPONSIVE BREAKPOINTS
// ===========================================

const char CSS_RESPONSIVE[] PROGMEM = R"rawliteral(
@media(min-width:700px){
body{display:flex;align-items:center;justify-content:center;padding:24px}
.wrap{width:90%;max-width:800px}
h1{font-size:1.8rem}
.sub{font-size:.8rem}
.status{padding:32px}
.status-text{font-size:2rem}
.card{padding:20px}
.card-title{font-size:.8rem;margin-bottom:14px}
.btn{padding:14px;font-size:.95rem}
.slider-label{font-size:.85rem}
.rot-row span{font-size:.85rem}
.rot-btn{padding:8px 16px;font-size:.8rem}
.stat{font-size:.95rem;padding:12px 0}
.footer{font-size:.75rem}
.effect-group-label{font-size:.65rem}
}
@media(min-width:1200px){
.wrap{width:80%;max-width:900px}
h1{font-size:2rem}
.status-text{font-size:2.2rem}
.btn{font-size:1rem}
.stat{font-size:1rem}
}
)rawliteral";

// ===========================================
// HELPER: Send all common styles
// ===========================================
// Use this in your page handlers:
//   server.sendContent("<style>");
//   sendCommonStyles();
//   server.sendContent("</style>");

inline void sendCommonStyles() {
  extern WebServer server;
  server.sendContent(FPSTR(CSS_BASE));
  server.sendContent(FPSTR(CSS_TYPOGRAPHY));
  server.sendContent(FPSTR(CSS_LAYOUT));
  server.sendContent(FPSTR(CSS_BUTTONS));
  server.sendContent(FPSTR(CSS_FORMS));
  server.sendContent(FPSTR(CSS_TOGGLE));
  server.sendContent(FPSTR(CSS_STATS));
  server.sendContent(FPSTR(CSS_STATUS));
  server.sendContent(FPSTR(CSS_FOOTER));
  server.sendContent(FPSTR(CSS_MODAL));
  server.sendContent(FPSTR(CSS_RESPONSIVE));
}

// For pages that need effect-specific styles too:
inline void sendDashboardStyles() {
  sendCommonStyles();
  extern WebServer server;
  server.sendContent(FPSTR(CSS_EFFECTS));
}

#endif // UI_STYLES_H
