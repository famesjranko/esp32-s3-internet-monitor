#ifndef UI_PORTAL_H
#define UI_PORTAL_H

/**
 * @file ui_portal.h
 * @brief Configuration portal HTML/CSS for WiFi setup
 * 
 * UI for the captive portal shown during initial device setup.
 * Displays available WiFi networks and credential input form.
 */

// Portal CSS - matches existing dark theme
const char PORTAL_CSS[] PROGMEM = R"rawliteral(
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,system-ui,sans-serif;background:#0f0f1a;color:#b8b8c8;padding:16px;min-height:100vh}
.wrap{max-width:400px;margin:0 auto}
h1{font-size:1.4rem;font-weight:600;margin-bottom:4px;color:#c8c8d8;text-align:center}
.sub{font-size:.7rem;color:#505068;letter-spacing:1px;text-align:center;margin-bottom:20px}
.card{background:#1a1a2e;border:1px solid #252540;border-radius:12px;padding:16px;margin-bottom:12px}
.card-title{font-size:.7rem;color:#505068;text-transform:uppercase;letter-spacing:1px;margin-bottom:12px}
.network{display:flex;justify-content:space-between;align-items:center;padding:12px;margin:4px 0;background:#252540;border-radius:8px;cursor:pointer;border:2px solid transparent;transition:all .15s}
.network:hover{background:#303050}
.network.selected{border-color:#6366f1;background:#303050}
.ssid{font-size:.9rem;color:#c8c8d8;word-break:break-all}
.meta{display:flex;gap:8px;align-items:center;flex-shrink:0;margin-left:8px}
.sig{font-family:monospace;color:#22c55e;font-size:.8rem}
.lock{color:#f59e0b;font-size:.7rem}
.no-networks{text-align:center;padding:20px;color:#707088}
input[type=password]{width:100%;padding:12px;border:1px solid #303048;border-radius:8px;background:#252540;color:#c8c8d8;font-size:1rem;margin:8px 0}
input:focus{outline:none;border-color:#6366f1}
input:-webkit-autofill{-webkit-box-shadow:0 0 0 1000px #252540 inset!important;-webkit-text-fill-color:#c8c8d8!important}
.btn{width:100%;padding:12px;border:none;border-radius:8px;font-size:.9rem;font-weight:600;cursor:pointer;transition:background .15s;margin-top:8px}
.btn.scan{background:#303048;color:#808098}
.btn.scan:hover{background:#404060}
.btn.connect{background:#4338ca;color:#d8d8e8}
.btn.connect:hover{background:#5145d6}
.btn:disabled{opacity:.5;cursor:not-allowed}
.status{text-align:center;padding:16px;font-size:.85rem;color:#707088}
.status.error{color:#ef4444}
.status.success{color:#22c55e}
#selssid{color:#c8c8d8;margin-bottom:8px;word-break:break-all}
.spinner{display:inline-block;width:16px;height:16px;border:2px solid #505068;border-top-color:#6366f1;border-radius:50%;animation:spin 1s linear infinite;vertical-align:middle;margin-right:8px}
@keyframes spin{to{transform:rotate(360deg)}}
)rawliteral";

// Portal JavaScript
const char PORTAL_JS[] PROGMEM = R"rawliteral(
let ssid='',isOpen=0,scanBtn=null;
function sel(s,o,e){
  ssid=s;isOpen=o;
  document.querySelectorAll('.network').forEach(n=>n.classList.remove('selected'));
  if(e&&e.currentTarget)e.currentTarget.classList.add('selected');
  if(o){
    document.getElementById('pwcard').style.display='none';
    document.getElementById('status').textContent='Selected open network: '+s;
  }else{
    document.getElementById('pwcard').style.display='block';
    document.getElementById('selssid').textContent='Network: '+s;
    document.getElementById('pw').value='';
    document.getElementById('pw').focus();
  }
}
function scan(e){
  const btn=e?e.currentTarget:scanBtn;
  if(!btn)return;
  scanBtn=btn;
  btn.disabled=true;
  btn.innerHTML='<span class="spinner"></span>Scanning...';
  document.getElementById('status').textContent='';
  function poll(){
    fetch('/scan').then(r=>r.text()).then(h=>{
      document.getElementById('networks').innerHTML=h;
      // Check for scanning indicator (spinner class or no network entries)
      const stillScanning=h.indexOf('spinner')>-1||h.indexOf('Scanning')>-1;
      if(stillScanning){
        setTimeout(poll,500);
      }else{
        btn.disabled=false;
        btn.textContent='Scan Again';
        ssid='';isOpen=0;
        document.getElementById('pwcard').style.display='none';
      }
    }).catch(()=>{
      document.getElementById('status').textContent='Scan failed';
      document.getElementById('status').className='status error';
      btn.disabled=false;
      btn.textContent='Scan Again';
    });
  }
  poll();
}
function connect(){
  if(!ssid){document.getElementById('status').textContent='Select a network first';document.getElementById('status').className='status error';return;}
  const pw=document.getElementById('pw')?.value||'';
  if(!isOpen&&!pw){document.getElementById('status').textContent='Enter WiFi password';document.getElementById('status').className='status error';return;}
  const adminPw=document.getElementById('adminpw')?.value||'admin';
  const btn=document.querySelector('.btn.connect');
  btn.disabled=true;
  btn.innerHTML='<span class="spinner"></span>Connecting...';
  document.getElementById('status').innerHTML='<span class="spinner"></span>Connecting to '+ssid+'...';
  document.getElementById('status').className='status';
  fetch('/connect',{
    method:'POST',
    headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(ssid)+'&password='+encodeURIComponent(pw)+'&adminpw='+encodeURIComponent(adminPw)
  }).then(r=>r.json()).then(d=>{
    if(d.success){
      document.getElementById('status').innerHTML='Connected!<br>IP: '+d.ip+'<br><br>Device will restart...<br><br>Login with password: '+(adminPw||'admin');
      document.getElementById('status').className='status success';
      btn.textContent='Connected!';
    }else{
      document.getElementById('status').textContent='Failed: '+(d.error||'Connection failed');
      document.getElementById('status').className='status error';
      btn.disabled=false;
      btn.textContent='Connect';
    }
  }).catch(()=>{
    document.getElementById('status').textContent='Connection error';
    document.getElementById('status').className='status error';
    btn.disabled=false;
    btn.textContent='Connect';
  });
}
document.addEventListener('keyup',e=>{if(e.key==='Enter'&&ssid)connect();});
)rawliteral";

#endif
