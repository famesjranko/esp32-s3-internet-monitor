#ifndef UI_DASHBOARD_EXTENDED_H
#define UI_DASHBOARD_EXTENDED_H

/**
 * @file ui_dashboard_extended.h
 * @brief Extended dashboard CSS for additional effects
 * 
 * Alternative dashboard styles with wider layout to accommodate
 * more effect buttons. Used when NUM_EFFECTS exceeds standard grid.
 */

// Dashboard CSS - Updated for more effects
const char DASHBOARD_CSS[] PROGMEM = R"rawliteral(
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:-apple-system,system-ui,sans-serif;background:#0f0f1a;color:#b8b8c8;padding:16px;min-height:100vh}
    .wrap{max-width:480px;margin:0 auto}
    .hdr{display:flex;justify-content:space-between;align-items:center;margin-bottom:16px}
    .hdr-left{text-align:left}
    h1{font-size:1.4rem;font-weight:600;margin-bottom:4px;color:#c8c8d8}
    .sub{font-size:.7rem;color:#505068;letter-spacing:1px}
    .logout{padding:6px 12px;border:1px solid #303048;border-radius:6px;background:#252540;color:#808098;font-size:.65rem;cursor:pointer}
    .logout:hover{background:#303050;color:#c8c8d8}
    .status{text-align:center;padding:24px;border-radius:12px;margin-bottom:14px;background:#1a1a2e;border:1px solid #252540}
    .status-dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:10px;animation:pulse 2s infinite;vertical-align:middle}
    @keyframes pulse{0%,100%{opacity:1}50%{opacity:.5}}
    .status-text{font-size:1.6rem;font-weight:700;letter-spacing:2px;vertical-align:middle}
    .card{background:#1a1a2e;border:1px solid #252540;border-radius:12px;padding:16px;margin-bottom:12px}
    .card-title{font-size:.7rem;color:#505068;text-transform:uppercase;letter-spacing:1px;margin-bottom:12px}
    .effect-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:6px}
    .btn{padding:8px 4px;border:1px solid #303048;border-radius:8px;background:#252540;color:#808098;font-size:.7rem;cursor:pointer;transition:all .15s;text-align:center;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
    .btn:hover{background:#303050;color:#c8c8d8}
    .btn.active{background:#4338ca;border-color:#4338ca;color:#d8d8e8}
    .btn.off.active{background:#b91c1c;border-color:#b91c1c}
    .cat-label{font-size:.6rem;color:#606080;text-transform:uppercase;letter-spacing:1px;margin:8px 0 4px;grid-column:1/-1}
    .slider-row{margin-top:14px;padding-top:14px;border-top:1px solid #252540}
    .slider-label{display:flex;justify-content:space-between;font-size:.75rem;color:#707088;margin-bottom:8px}
    .slider-val{color:#b8b8c8;font-weight:600}
    input[type=range]{width:100%;height:4px;border-radius:2px;background:#303048;-webkit-appearance:none;cursor:pointer}
    input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:16px;height:16px;border-radius:50%;background:#6366f1}
    .rot-row{display:flex;gap:6px;margin-top:14px;padding-top:14px;border-top:1px solid #252540;align-items:center}
    .rot-row span{font-size:.75rem;color:#707088;margin-right:auto}
    .rot-btn{padding:6px 12px;border:1px solid #303048;border-radius:6px;background:#252540;color:#808098;font-size:.7rem;cursor:pointer}
    .rot-btn.active{background:#4338ca;border-color:#4338ca;color:#d8d8e8}
    .stat{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #252540;font-size:.8rem}
    .stat:last-child{border:none}
    .stat-label{color:#707088}
    .stat-val{font-weight:600;color:#b8b8c8}
    .good{color:#22c55e}.bad{color:#ef4444}
    .btn-danger{background:#7f1d1d;border-color:#991b1b;color:#fca5a5;margin-top:14px;padding-top:14px;border-top:1px solid #252540}
    .btn-danger:hover{background:#991b1b;color:#fef2f2}
    .footer{text-align:center;padding:14px;font-size:.65rem;color:#505068}
    .collapse-btn{width:100%;padding:8px;background:#252540;border:1px solid #303048;border-radius:8px;color:#808098;cursor:pointer;font-size:.75rem;margin-top:8px;display:flex;justify-content:space-between;align-items:center}
    .collapse-btn:hover{background:#303050}
    .collapse-content{display:none;margin-top:8px}
    .collapse-content.show{display:block}
    .arrow{transition:transform .2s}
    .arrow.up{transform:rotate(180deg)}
    @media(min-width:700px){
      body{display:flex;align-items:center;justify-content:center;padding:24px}
      .wrap{width:90%;max-width:800px}
      h1{font-size:1.8rem}
      .sub{font-size:.8rem}
      .status{padding:32px}
      .status-text{font-size:2rem}
      .card{padding:20px}
      .card-title{font-size:.8rem;margin-bottom:14px}
      .effect-grid{grid-template-columns:repeat(6,1fr)}
      .btn{padding:12px 8px;font-size:.8rem}
      .slider-label{font-size:.85rem}
      .rot-row span{font-size:.85rem}
      .rot-btn{padding:8px 16px;font-size:.8rem}
      .stat{font-size:.95rem;padding:12px 0}
      .footer{font-size:.75rem}
    }
    @media(min-width:1200px){
      .wrap{width:80%;max-width:900px}
      h1{font-size:2rem}
      .status-text{font-size:2.2rem}
      .effect-grid{grid-template-columns:repeat(8,1fr)}
      .btn{font-size:.85rem}
      .stat{font-size:1rem}
    }
)rawliteral";

// Dashboard JavaScript (uses cookies for auth)
const char DASHBOARD_JS[] PROGMEM = R"rawliteral(
    function E(e){fetch('/effect?e='+e,{credentials:'same-origin'}).then(r=>r.json()).then(d=>{document.getElementById('bv').textContent=d.brightness+'/50';document.getElementById('sv').textContent=d.speed+'%';document.querySelector('input[oninput*="B("]').value=d.brightness;document.querySelector('input[oninput*="S("]').value=d.speed}).catch(()=>{});document.querySelectorAll('.effect-grid .btn').forEach((b)=>{let be=parseInt(b.dataset.effect);b.classList.toggle('active',be===e)})}
    function B(v){document.getElementById('bv').textContent=v+'/50';fetch('/brightness?b='+v,{credentials:'same-origin'})}
    function S(v){document.getElementById('sv').textContent=v+'%';fetch('/speed?s='+v,{credentials:'same-origin'})}
    function R(r){fetch('/rotation?r='+r,{credentials:'same-origin'});document.querySelectorAll('.rot-btn').forEach((b,i)=>b.classList.toggle('active',i===r))}
    function logout(){fetch('/logout',{credentials:'same-origin'}).then(()=>window.location='/');}
    function resetWifi(){if(confirm('Reset WiFi settings?\n\nDevice will reboot into setup mode.')){fetch('/reset-wifi',{credentials:'same-origin'}).then(r=>r.json()).then(d=>{alert('To reconfigure:\n\n1. Connect to WiFi: '+d.ssid+'\n2. Go to http://192.168.4.1\n\nRebooting...')}).catch(()=>alert('Rebooting...'))}}
    function toggleMore(){let c=document.getElementById('moreEffects');let a=document.getElementById('moreArrow');c.classList.toggle('show');a.classList.toggle('up')}
    function fmt(ms){let s=Math.floor(ms/1000),m=Math.floor(s/60),h=Math.floor(m/60),d=Math.floor(h/24);let r='';if(d)r+=d+'d ';if(h%24)r+=(h%24)+'h ';if(m%60)r+=(m%60)+'m ';r+=(s%60)+'s';return r}
    const colors={4:'#22c55e',5:'#f59e0b',6:'#ef4444',3:'#ef4444',2:'#c026d3',0:'#3b82f6',1:'#3b82f6'};
    function upd(){fetch('/stats',{credentials:'same-origin'}).then(r=>{if(!r.ok){window.location='/';throw'';}return r.json()}).then(d=>{
      document.getElementById('up').textContent=fmt(d.uptime);
      document.getElementById('chk').textContent=d.checks;
      const rate=document.getElementById('rate');rate.textContent=d.rate+'%';rate.className='stat-val '+(d.rate>95?'good':'bad');
      const fail=document.getElementById('fail');fail.textContent=d.failed;fail.className='stat-val '+(d.failed>0?'bad':'');
      document.getElementById('last').textContent=d.lastOutage>0?fmt(d.lastOutage):'None';
      document.getElementById('down').textContent=fmt(d.downtime);
      const rssi=document.getElementById('rssi');rssi.textContent=d.rssi+' dBm';rssi.className='stat-val '+(d.rssi>-60?'good':'');
      document.getElementById('heap').textContent=Math.floor(d.heap/1024)+' KB';
      document.getElementById('temp').textContent=d.temp+'°C';
      const c=colors[d.state]||'#3b82f6';
      document.getElementById('dot').style.background=c;document.getElementById('dot').style.boxShadow='0 0 8px '+c;
      document.getElementById('stxt').style.color=c;document.getElementById('stxt').textContent=d.stateText;
      if(d.effect!==undefined){document.querySelectorAll('.effect-grid .btn').forEach((b)=>{let be=parseInt(b.dataset.effect);b.classList.toggle('active',be===d.effect)})}
    }).catch(()=>{})}
    setInterval(upd,5000);
)rawliteral";

// Effect button HTML generator - pass current effect to highlight
// This generates the effect buttons dynamically based on the effect names
const char DASHBOARD_EFFECTS_HTML[] PROGMEM = R"rawliteral(
<div class="effect-grid">
  <button class="btn off" data-effect="0" onclick="E(0)">Off</button>
  <button class="btn" data-effect="1" onclick="E(1)">Solid</button>
  <button class="btn" data-effect="2" onclick="E(2)">Ripple</button>
  <button class="btn" data-effect="3" onclick="E(3)">Rainbow</button>
  <button class="btn" data-effect="4" onclick="E(4)">Pulse</button>
  <button class="btn" data-effect="5" onclick="E(5)">Rain</button>
  <button class="btn" data-effect="6" onclick="E(6)">❤️ Heart</button>
  <button class="btn" data-effect="7" onclick="E(7)">🟡 Pac-Man</button>
</div>
<button class="collapse-btn" onclick="toggleMore()">
  More Effects (18) <span id="moreArrow" class="arrow">▼</span>
</button>
<div id="moreEffects" class="collapse-content">
  <div class="effect-grid">
    <div class="cat-label">🎮 Retro</div>
    <button class="btn" data-effect="8" onclick="E(8)">👻 Ghost</button>
    <button class="btn" data-effect="9" onclick="E(9)">👾 Invader</button>
    <button class="btn" data-effect="17" onclick="E(17)">🐍 Snake</button>
    <button class="btn" data-effect="25" onclick="E(25)">💚 Creeper</button>
    
    <div class="cat-label">✨ Visual</div>
    <button class="btn" data-effect="10" onclick="E(10)">🔥 Fire</button>
    <button class="btn" data-effect="11" onclick="E(11)">💻 Matrix</button>
    <button class="btn" data-effect="13" onclick="E(13)">🌈 Plasma</button>
    <button class="btn" data-effect="15" onclick="E(15)">⭐ Stars</button>
    
    <div class="cat-label">🔄 Animated</div>
    <button class="btn" data-effect="12" onclick="E(12)">🧬 Life</button>
    <button class="btn" data-effect="14" onclick="E(14)">🌀 Spiral</button>
    <button class="btn" data-effect="18" onclick="E(18)">⚽ Ball</button>
    <button class="btn" data-effect="19" onclick="E(19)">◎ Rings</button>
    
    <div class="cat-label">😊 Fun</div>
    <button class="btn" data-effect="16" onclick="E(16)">📶 WiFi</button>
    <button class="btn" data-effect="20" onclick="E(20)">♟️ Check</button>
    <button class="btn" data-effect="21" onclick="E(21)">✨ Sparkle</button>
    <button class="btn" data-effect="22" onclick="E(22)">😊 Smiley</button>
    <button class="btn" data-effect="23" onclick="E(23)">⭐ Star</button>
    <button class="btn" data-effect="24" onclick="E(24)">📊 Wipe</button>
  </div>
</div>
)rawliteral";

#endif // UI_DASHBOARD_EXTENDED_H
