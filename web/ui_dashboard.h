#ifndef UI_DASHBOARD_H
#define UI_DASHBOARD_H

/**
 * @file ui_dashboard.h
 * @brief Main dashboard HTML/CSS/JS for web interface
 * 
 * Contains the primary dashboard UI including effect controls,
 * brightness/speed sliders, rotation buttons, and status display.
 */

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
    .card-title.collapsible{cursor:pointer;display:flex;justify-content:space-between;align-items:center;user-select:none}
    .card-title.collapsible:hover{color:#808098}
    .card-title .toggle{font-size:1rem;transition:transform .2s}
    .card-title.collapsed .toggle{transform:rotate(-90deg)}
    .card-body{overflow:hidden;transition:max-height .3s ease-out;max-height:1000px}
    .card-body.collapsed{max-height:0;margin:0;padding:0}
    .grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
    .btn{padding:10px;border:1px solid #303048;border-radius:8px;background:#252540;color:#808098;font-size:.8rem;cursor:pointer;transition:all .15s}
    .btn:hover{background:#303050;color:#c8c8d8}
    .btn.active{background:#4338ca;border-color:#4338ca;color:#d8d8e8}
    .btn.off.active{background:#b91c1c;border-color:#b91c1c}
    .effect-group{margin-bottom:10px}
    .effect-group-label{font-size:.6rem;color:#505068;margin-bottom:6px;text-transform:uppercase;letter-spacing:.5px}
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
    .tog{position:relative;display:inline-block;width:40px;height:20px;cursor:pointer}
    .tog-bg{position:absolute;top:0;left:0;right:0;bottom:0;border-radius:20px;transition:background .2s}
    .tog-knob{position:absolute;height:16px;width:16px;bottom:2px;background:#fff;border-radius:50%;transition:left .2s}
)rawliteral";

// Dashboard JavaScript (uses cookies for auth)
const char DASHBOARD_JS[] PROGMEM = R"rawliteral(
    function E(e){fetch('/effect?e='+e,{credentials:'same-origin'}).then(r=>r.json()).then(d=>{document.getElementById('bv').textContent=d.brightness+'/50';document.getElementById('sv').textContent=d.speed+'%';document.querySelector('input[oninput*="B("]').value=d.brightness;document.querySelector('input[oninput*="S("]').value=d.speed}).catch(err=>console.warn('Effect error:',err));document.querySelectorAll('.grid .btn').forEach(b=>{const n=parseInt(b.getAttribute('onclick').match(/\d+/)[0]);b.classList.toggle('active',n===e)})}
    function B(v){const el=document.getElementById('bv');el.textContent=v+'/50';fetch('/brightness?b='+v,{credentials:'same-origin'}).catch(()=>{el.textContent='Error';setTimeout(upd,500)})}
    function S(v){const el=document.getElementById('sv');el.textContent=v+'%';fetch('/speed?s='+v,{credentials:'same-origin'}).catch(()=>{el.textContent='Error';setTimeout(upd,500)})}
    function R(r){fetch('/rotation?r='+r,{credentials:'same-origin'}).catch(err=>console.warn('Rotation error:',err));document.querySelectorAll('.rot-btn').forEach((b,i)=>b.classList.toggle('active',i===r))}
    function T(id){const t=document.getElementById(id+'T'),b=document.getElementById(id+'B');if(t&&b){t.classList.toggle('collapsed');b.classList.toggle('collapsed');localStorage.setItem(id,t.classList.contains('collapsed')?'1':'0')}}
    function logout(){fetch('/logout',{credentials:'same-origin'}).then(()=>window.location='/');}
    function factoryReset(){
      showConfirm('This will clear ALL settings:\n\n• WiFi network & password\n• Dashboard password (reset to: admin)\n• Brightness, effect, speed, rotation\n• MQTT configuration\n\nDevice will reboot into setup mode.',{
        title:'Factory Reset',
        confirmText:'Reset Everything',
        cancelText:'Cancel',
        danger:true,
        callback:function(confirmed){
          if(confirmed){
            fetch('/factory-reset',{credentials:'same-origin'}).then(r=>r.json()).then(d=>{
              showModal({
                title:'Settings Cleared',
                message:'To reconfigure:\n\n1. Connect to WiFi: '+d.ssid+'\n2. Go to: http://192.168.4.1\n3. Login with password: admin\n\nRebooting now...',
                buttons:[{text:'OK',class:'primary'}]
              });
            }).catch(()=>showAlert('Rebooting...'));
          }
        }
      });
    }
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
      const minheap=document.getElementById('minheap');if(minheap&&d.minHeap!=null)minheap.textContent=Math.floor(d.minHeap/1024)+' KB';
      document.getElementById('temp').textContent=d.temp+'°C';
      const c=colors[d.state]||'#3b82f6';
      document.getElementById('dot').style.background=c;document.getElementById('dot').style.boxShadow='0 0 8px '+c;
      document.getElementById('stxt').style.color=c;document.getElementById('stxt').textContent=d.stateText;
      // Performance stats (with null checks)
      const fps=document.getElementById('fps');if(fps&&d.ledFps!=null){fps.textContent=d.ledFps.toFixed(1);fps.className='stat-val '+(d.ledFps>55?'good':(d.ledFps>30?'':'bad'));}
      const frameus=document.getElementById('frameus');if(frameus&&d.ledFrameUs!=null)frameus.textContent=d.ledFrameUs+' µs';
      const maxf=document.getElementById('maxframeus');if(maxf&&d.ledMaxFrameUs!=null){maxf.textContent=d.ledMaxFrameUs+' µs';maxf.className='stat-val '+(d.ledMaxFrameUs<10000?'good':(d.ledMaxFrameUs<16000?'':'bad'));}
      const ledstack=document.getElementById('ledstack');if(ledstack&&d.ledStack!=null)ledstack.textContent=d.ledStack+' bytes';
      const netstack=document.getElementById('netstack');if(netstack&&d.netStack!=null)netstack.textContent=d.netStack+' bytes';
    }).catch(()=>{})}
    // Restore collapsed state from localStorage (effects defaults open, sys/diag default collapsed)
    ['effects'].forEach(id=>{if(localStorage.getItem(id)==='1'){const t=document.getElementById(id+'T'),b=document.getElementById(id+'B');if(t&&b){t.classList.add('collapsed');b.classList.add('collapsed')}}});
    ['sys','diag','mqtt'].forEach(id=>{if(localStorage.getItem(id)==='0'){const t=document.getElementById(id+'T'),b=document.getElementById(id+'B');if(t&&b){t.classList.remove('collapsed');b.classList.remove('collapsed')}}});
    setInterval(upd,2000);upd();
    
    // MQTT Functions
    function mqttToggle(){
      const inp=document.getElementById('mqttEn');
      const en=inp.value!=='1';
      inp.value=en?'1':'0';
      document.getElementById('mqttEnBg').style.background=en?'#4338ca':'#303048';
      document.getElementById('mqttEnKnob').style.left=en?'22px':'2px';
      const d=new FormData();d.append('enabled',en?'1':'0');
      fetch('/mqtt/config',{method:'POST',body:d,credentials:'same-origin'}).then(r=>r.json()).then(r=>{
        const s=document.getElementById('mqttStatus');if(s){s.textContent=r.status;s.style.color=r.connected?'#22c55e':(en?'#f59e0b':'#707088');}
      }).catch(err=>console.warn('MQTT toggle error:',err));
    }
    function togHA(){
      const inp=document.getElementById('mqttHA');
      const en=inp.value!=='1';
      inp.value=en?'1':'0';
      document.getElementById('mqttHABg').style.background=en?'#4338ca':'#303048';
      document.getElementById('mqttHAKnob').style.left=en?'22px':'2px';
    }
    function mqttSave(){
      const d=new FormData();
      d.append('enabled',document.getElementById('mqttEn').value);
      d.append('broker',document.getElementById('mqttBroker').value);
      d.append('port',document.getElementById('mqttPort').value);
      d.append('username',document.getElementById('mqttUser').value);
      d.append('password',document.getElementById('mqttPass').value);
      d.append('topic',document.getElementById('mqttTopic').value);
      d.append('interval',document.getElementById('mqttInt').value);
      d.append('ha_discovery',document.getElementById('mqttHA').value);
      fetch('/mqtt/config',{method:'POST',body:d,credentials:'same-origin'}).then(r=>r.json()).then(r=>{
        const s=document.getElementById('mqttStatus');if(s){s.textContent=r.status;s.style.color=r.connected?'#22c55e':'#f59e0b';}
        if(r.success)showSuccess('MQTT settings saved!','Settings Saved');
        else showError('Error saving settings','Save Failed');
      }).catch(()=>showError('Error saving MQTT settings','Save Failed'));
    }
    function mqttTest(){
      const s=document.getElementById('mqttStatus');
      if(s){s.textContent='Testing...';s.style.color='#f59e0b';}
      fetch('/mqtt/test',{method:'POST',credentials:'same-origin'}).then(r=>r.json()).then(r=>{
        if(s){s.textContent=r.success?'Connected':'Failed';s.style.color=r.success?'#22c55e':'#ef4444';}
        if(r.success)showSuccess(r.message,'Connection Test');
        else showError(r.message,'Connection Test');
      }).catch(()=>{
        if(s){s.textContent='Error';s.style.color='#ef4444';}
        showError('Connection test failed','Test Failed');
      });
    }
    // Update MQTT status periodically
    function updMqtt(){fetch('/mqtt/status',{credentials:'same-origin'}).then(r=>r.json()).then(d=>{
      const s=document.getElementById('mqttStatus');if(s){s.textContent=d.status;s.style.color=d.connected?'#22c55e':(d.enabled?'#f59e0b':'#707088');}
      // Sync enabled toggle if it changed externally
      const inp=document.getElementById('mqttEn');
      if(inp&&((inp.value==='1')!==d.enabled)){
        inp.value=d.enabled?'1':'0';
        const bg=document.getElementById('mqttEnBg'),knob=document.getElementById('mqttEnKnob');
        if(bg)bg.style.background=d.enabled?'#4338ca':'#303048';
        if(knob)knob.style.left=d.enabled?'22px':'2px';
      }
    }).catch(()=>{});}
    setInterval(updMqtt,5000);
)rawliteral";

#endif
