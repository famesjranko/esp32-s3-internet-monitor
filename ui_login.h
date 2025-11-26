#ifndef UI_LOGIN_H
#define UI_LOGIN_H

const char LOGIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Internet Monitor - Login</title>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:-apple-system,system-ui,sans-serif;background:#0f0f1a;color:#b8b8c8;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
    .login{width:100%;max-width:320px;text-align:center}
    h1{font-size:1.4rem;font-weight:600;margin-bottom:8px;color:#c8c8d8}
    .sub{font-size:.7rem;color:#505068;letter-spacing:1px;margin-bottom:32px}
    .card{background:#1a1a2e;border:1px solid #252540;border-radius:12px;padding:24px}
    .icon{font-size:2.5rem;margin-bottom:16px}
    label{display:block;font-size:.7rem;color:#707088;text-transform:uppercase;letter-spacing:1px;margin-bottom:8px;text-align:left}
    input[type=password]{width:100%;padding:12px;border:1px solid #303048;border-radius:8px;background:#252540;color:#c8c8d8;font-size:1rem;margin-bottom:16px}
    input[type=password]:focus{outline:none;border-color:#4338ca}
    button{width:100%;padding:12px;border:none;border-radius:8px;background:#4338ca;color:#d8d8e8;font-size:.9rem;font-weight:600;cursor:pointer;transition:background .15s}
    button:hover{background:#5145d6}
    .error{color:#ef4444;font-size:.75rem;margin-bottom:12px;display:none}
    .error.show{display:block}
  </style>
</head>
<body>
  <div class="login">
    <h1>Internet Monitor</h1>
    <p class="sub">ESP32-S3 MATRIX</p>
    <div class="card">
      <div class="icon">🔐</div>
      <label>Password</label>
      <input type="password" id="pw" placeholder="Enter password" autofocus>
      <div class="error" id="err">Invalid password</div>
      <button onclick="login()">Login</button>
    </div>
  </div>
  <script>
    document.getElementById('pw').addEventListener('keyup',e=>{if(e.key==='Enter')login()});
    function login(){
      const pw=document.getElementById('pw').value;
      const err=document.getElementById('err');
      err.classList.remove('show');
      fetch('/login',{
        method:'POST',
        headers:{'Content-Type':'application/x-www-form-urlencoded'},
        body:'password='+encodeURIComponent(pw)
      })
        .then(r=>r.json().then(d=>({ok:r.ok,data:d})))
        .then(({ok,data})=>{
          if(ok){
            window.location='/';
          }else{
            if(data.retry_after){
              err.textContent='Too many attempts. Try again in '+data.retry_after+'s';
            }else if(data.attempts!==undefined){
              err.textContent='Invalid password ('+data.attempts+' attempts left)';
            }else{
              err.textContent='Invalid password';
            }
            err.classList.add('show');
          }
        })
        .catch(()=>{err.textContent='Connection error';err.classList.add('show');});
    }
  </script>
</body>
</html>
)rawliteral";

#endif
