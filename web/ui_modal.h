#ifndef UI_MODAL_H
#define UI_MODAL_H

/**
 * @file ui_modal.h
 * @brief Modal dialog system for alerts, confirmations, and notifications
 * 
 * Provides styled modal dialogs replacing browser alert/confirm dialogs.
 * Includes showModal(), showAlert(), showSuccess(), showError(), showConfirm().
 */

// ===========================================
// MODAL HTML (add to page body)
// ===========================================

const char MODAL_HTML[] PROGMEM = R"rawliteral(
<div class="modal-overlay" id="modal">
  <div class="modal">
    <div class="modal-title" id="modalTitle"></div>
    <div class="modal-body" id="modalBody"></div>
    <div class="modal-footer" id="modalFooter"></div>
  </div>
</div>
)rawliteral";

// ===========================================
// MODAL JAVASCRIPT
// ===========================================

const char MODAL_JS[] PROGMEM = R"rawliteral(
var modalCallback=null;
function showModal(opts){
  const m=document.getElementById('modal');
  const title=document.getElementById('modalTitle');
  const body=document.getElementById('modalBody');
  const footer=document.getElementById('modalFooter');
  
  // Set content
  title.textContent=opts.title||'';
  title.style.display=opts.title?'block':'none';
  body.textContent=opts.message||'';
  
  // Build buttons
  footer.innerHTML='';
  if(opts.buttons){
    opts.buttons.forEach(btn=>{
      const b=document.createElement('button');
      b.className='modal-btn'+(btn.class?' '+btn.class:'');
      b.textContent=btn.text;
      b.onclick=function(){
        hideModal();
        if(btn.action)btn.action();
        if(modalCallback)modalCallback(btn.value);
      };
      footer.appendChild(b);
    });
  }
  
  modalCallback=opts.callback||null;
  m.classList.add('show');
  
  // Close on overlay click (optional)
  if(opts.closeOnOverlay!==false){
    m.onclick=function(e){if(e.target===m)hideModal();}
  }
}
function hideModal(){
  document.getElementById('modal').classList.remove('show');
  modalCallback=null;
}
function showAlert(message,opts){
  opts=opts||{};
  showModal({
    title:opts.title||'',
    message:message,
    buttons:[{text:'OK',class:'primary',value:true}],
    callback:opts.callback
  });
}
function showSuccess(message,title){
  showModal({
    title:title||'Success',
    message:message,
    buttons:[{text:'OK',class:'primary',value:true}]
  });
}
function showError(message,title){
  showModal({
    title:title||'Error',
    message:message,
    buttons:[{text:'OK',class:'primary',value:true}]
  });
}
function showConfirm(message,opts){
  opts=opts||{};
  showModal({
    title:opts.title||'Confirm',
    message:message,
    buttons:[
      {text:opts.cancelText||'Cancel',value:false},
      {text:opts.confirmText||'OK',class:opts.danger?'danger':'primary',value:true}
    ],
    callback:opts.callback,
    closeOnOverlay:false
  });
}
)rawliteral";

#endif // UI_MODAL_H
