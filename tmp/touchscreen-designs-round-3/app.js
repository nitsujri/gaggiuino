(function(){
  'use strict';
  const {Model,concepts,clone}=window.GaggiuinoTrial,{View}=window.GaggiuinoView;
  const single=document.body.dataset.concept,all=single===''||single===undefined;
  const selection=all?concepts:[concepts[Number(single)]];const models=[],views=[];
  const gallery=document.getElementById('designs');
  selection.forEach(concept=>{
    const model=new Model(window.GAGGIUINO_BACKUP,concept.number-1);
    try{const stored=JSON.parse(localStorage.getItem('gaggiuino-lagoon-trial-'+model.concept));if(stored&&stored.cfg?.profiles?.length===5){model.cfg=clone(stored.cfg);model.saved=clone(stored.cfg);model.prefs={...model.prefs,...stored.prefs};model.savedPrefs=clone(model.prefs);}}catch(e){}
    model.editProfile=model.cfg.activeProfile;models.push(model);
    const card=document.createElement('article');card.className=all?'design-card':'design-card single-card';card.dataset.design=concept.id;
    const header=document.createElement('header');header.innerHTML=`<span class="card-number">${String(concept.number).padStart(2,'0')} / 10</span><h2>${concept.name}</h2><p>${concept.description}</p>`;
    const frame=document.createElement('div');frame.className='screen-container';const screen=document.createElement('div');screen.className='screen';frame.append(screen);
    const footer=document.createElement('div');footer.className='card-footer';footer.innerHTML=`<span>320 × 240 · RGB565</span><a href="${String(concept.number).padStart(2,'0')}-${concept.id}.html">${all?'Open larger trial':'Permalink'}</a>`;
    card.append(header,frame,footer);gallery.append(card);const view=new View(screen,model);views.push(view);
    if(!all){document.getElementById('previous-design').href=String((concept.number+8)%10+1).padStart(2,'0')+'-'+concepts[(concept.number+8)%10].id+'.html';document.getElementById('next-design').href=String(concept.number%10+1).padStart(2,'0')+'-'+concepts[concept.number%10].id+'.html';}
  });
  function redraw(){views.forEach(v=>v.draw());status();}
  function status(){const m=models[0],reason=m.flushReason();document.getElementById('lab-status').textContent=`${m.isFlushing()?'FLUSH OUTPUT ON':'FLUSH OUTPUT OFF'}\nGross: ${Number.isFinite(m.gross())?m.gross().toFixed(1):'--'} g · net ${Number.isFinite(m.weight())?m.weight().toFixed(1):'--'} g\n${reason||'Tray guard ready'}\nTank: ${m.water()===null?'--':m.water()+'%'}`;}
  function sensors(changes){models.forEach(m=>m.setLab(changes));redraw();}
  const tray=document.getElementById('tray-present'),cup=document.getElementById('cup-present'),scale=document.getElementById('scale-status'),distance=document.getElementById('distance'),tof=document.getElementById('tof-valid');
  tray.onchange=()=>{if(!tray.checked)cup.checked=false;sensors({tray:tray.checked,cup:cup.checked?120:0});};cup.onchange=()=>sensors({cup:cup.checked?120:0});
  scale.onchange=()=>{models.forEach(m=>{m.prefs.trayCalibrated=scale.value!=='unconfigured';m.setLab({scales:scale.value==='unconfigured'?'ok':scale.value});});redraw();};
  distance.oninput=()=>{document.getElementById('distance-value').textContent=distance.value+' mm';sensors({distance:Number(distance.value)});};
  tof.onchange=()=>sensors({tofValid:tof.checked});
  const temperature=document.getElementById('temperature');
  temperature.oninput=()=>{document.getElementById('temperature-value').textContent=Number(temperature.value).toFixed(1)+' °C';sensors({temperature:Number(temperature.value)});};
  document.getElementById('brew-temperature').onclick=()=>{temperature.value=93;temperature.oninput();};
  document.getElementById('tank-full').onclick=()=>{distance.value=42;distance.oninput();};document.getElementById('tank-low').onclick=()=>{distance.value=135;distance.oninput();};
  const brew=document.getElementById('brew-switch'),steam=document.getElementById('steam-switch');
  brew.onclick=()=>{const on=brew.getAttribute('aria-pressed')!=='true';brew.setAttribute('aria-pressed',String(on));brew.textContent=on?'Brew switch ON':'Brew switch OFF';models.forEach(m=>m.switchBrew(on));redraw();};
  steam.onclick=()=>{const on=steam.getAttribute('aria-pressed')!=='true';steam.setAttribute('aria-pressed',String(on));models.forEach(m=>{m.steam=on;if(on)m.stopFlush();});redraw();};
  document.getElementById('reset-trials').onclick=()=>{models.forEach((m,i)=>{try{localStorage.removeItem('gaggiuino-lagoon-trial-'+m.concept);}catch(e){}const fresh=new Model(window.GAGGIUINO_BACKUP,m.concept);Object.assign(m,fresh);});tray.checked=true;cup.checked=false;scale.value='ok';distance.value=62;tof.checked=true;temperature.value=74.6;document.getElementById('temperature-value').textContent='74.6 °C';brew.setAttribute('aria-pressed','false');brew.textContent='Brew switch OFF';steam.setAttribute('aria-pressed','false');document.getElementById('distance-value').textContent='62 mm';redraw();};
  document.getElementById('open-navigation').onclick=()=>{models.forEach(m=>m.go('menu'));redraw();};
  document.getElementById('open-manual').onclick=()=>{models.forEach(m=>m.go('manual'));redraw();};
  document.getElementById('open-water').onclick=()=>{models.forEach(m=>m.go('water'));redraw();};document.getElementById('open-home').onclick=()=>{models.forEach(m=>m.home());redraw();};
  if(!all){const zoom=document.getElementById('zoom');zoom.onchange=()=>document.documentElement.style.setProperty('--zoom',zoom.value);document.documentElement.style.setProperty('--zoom','2');}
  function download(name,contents,type){const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([contents],{type}));a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);}
  document.getElementById('export-settings').onclick=()=>download('gaggiuino-trial-settings.json',JSON.stringify({prototype:true,concept:concepts[models[0].concept].name,settings:models[0].cfg,calibration:models[0].prefs},null,2),'application/json');
  document.getElementById('export-image').onclick=()=>{const a=document.createElement('a');a.download=concepts[models[0].concept].id+'-320x240.png';a.href=views[0].canvas.toDataURL('image/png');a.click();};
  let last=performance.now();setInterval(()=>{const now=performance.now(),dt=Math.min(.5,(now-last)/1000);last=now;models.forEach(m=>m.tick(dt));redraw();},250);
  status();
})();
