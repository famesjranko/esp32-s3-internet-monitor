#ifndef UI_DASHBOARD_H
#define UI_DASHBOARD_H

// Dashboard CSS
const char DASHBOARD_CSS[] PROGMEM = R"rawliteral(
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:-apple-system,system-ui,sans-serif;background:#0f0f1a;color:#b8b8c8;padding:16px;min-height:100vh}
    .wrap{max-width:420px;margin:0 auto}
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
    .grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
    .btn{padding:10px;border:1px solid #303048;border-radius:8px;background:#252540;color:#808098;font-size:.8rem;cursor:pointer;transition:all .15s}
    .btn:hover{background:#303050;color:#c8c8d8}
    .btn.active{background:#4338ca;border-color:#4338ca;color:#d8d8e8}
    .btn.off.active{background:#b91c1c;border-color:#b91c1c}
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
    .footer{text-align:center;padding:14px;font-size:.65rem;color:#505068}
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
    }
    @media(min-width:1200px){
      .wrap{width:80%;max-width:900px}
      h1{font-size:2rem}
      .status-text{font-size:2.2rem}
      .btn{font-size:1rem}
      .stat{font-size:1rem}
    }
)rawliteral";

// Dashboard JavaScript (KEY placeholder will be replaced)
const char DASHBOARD_JS[] PROGMEM = R"rawliteral(
    const K='%KEY%';
    function q(p){return p+(p.includes('?')?'&':'?')+'key='+encodeURIComponent(K)}
    function E(e){fetch(q('/effect?e='+e));document.querySelectorAll('.grid .btn').forEach((b,i)=>b.classList.toggle('active',i===e))}
    function B(v){document.getElementById('bv').textContent=v+'/50';fetch(q('/brightness?b='+v))}
    function S(v){document.getElementById('sv').textContent=v+'%';fetch(q('/speed?s='+v))}
    function R(r){fetch(q('/rotation?r='+r));document.querySelectorAll('.rot-btn').forEach((b,i)=>b.classList.toggle('active',i===r))}
    function logout(){localStorage.removeItem('imkey');window.location='/';}
    function fmt(ms){let s=Math.floor(ms/1000),m=Math.floor(s/60),h=Math.floor(m/60),d=Math.floor(h/24);let r='';if(d)r+=d+'d ';if(h%24)r+=(h%24)+'h ';if(m%60)r+=(m%60)+'m ';r+=(s%60)+'s';return r}
    const colors={3:'#22c55e',4:'#f59e0b',5:'#ef4444',2:'#ef4444',0:'#3b82f6',1:'#3b82f6'};
    function upd(){fetch(q('/stats')).then(r=>{if(!r.ok)throw'';return r.json()}).then(d=>{
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
    }).catch(()=>{})}
    setInterval(upd,5000);
)rawliteral";

#endif
