const fs=require('fs'),vm=require('vm'),assert=require('assert/strict'),path=require('path');
const {Model,concepts,profileGroups,systemGroups,waterPercent}=require('./model.js');
const context={window:{}};vm.runInNewContext(fs.readFileSync(path.join(__dirname,'settings.js'),'utf8'),context);
const backup=JSON.parse(JSON.stringify(context.window.GAGGIUINO_BACKUP));let checks=0;
function check(f){f();checks++;}
check(()=>assert.equal(concepts.length,10));
check(()=>assert.equal(new Set(concepts.map(c=>c.id)).size,10));
check(()=>assert.deepEqual(Object.keys(backup.profiles[0]).sort(),['name',...Object.values(profileGroups).flat()].sort()));
check(()=>assert.deepEqual(Object.keys(backup).filter(k=>!['profiles','activeProfile'].includes(k)).sort(),Object.values(systemGroups).flat().filter(k=>k!=='brightness').sort()));
check(()=>{for(let full=1;full<125;full++){assert.equal(waterPercent(full,full),100);assert.equal(waterPercent(125,full),10);assert.equal(waterPercent(126,full),9);assert.equal(waterPercent(160,full),9);}});
check(()=>assert.equal(waterPercent(42,28,false),null));
check(()=>{const m=new Model(backup);m.setLab({distance:42});assert(m.water()<100);assert(m.setFull());assert.equal(m.water(),100);m.save();assert.equal(m.savedPrefs.fullDistance,42);m.setLab({distance:126});assert.equal(m.water(),9);});
check(()=>{const m=new Model(backup);m.setLab({distance:135});assert.equal(m.setFull(),false);assert.equal(m.prefs.fullDistance,30);});
for(const mode of ['offline','stale','invalid'])for(const cycle of [false,true]){
  check(()=>{const m=new Model(backup);m.setLab({scales:mode});assert.equal(m.flushToggle(cycle),false);assert(!m.isFlushing());});
  check(()=>{const m=new Model(backup);assert(m.flushToggle(cycle));m.setLab({scales:mode});assert(!m.isFlushing());assert.equal(m.popup.title,'Flush stopped');});
}
for(const cycle of [false,true]){
  check(()=>{const m=new Model(backup);m.setLab({tray:false});m.tare();m.scalesView=false;assert.equal(m.weight(),0);assert.equal(m.flushToggle(cycle),false);});
  check(()=>{const m=new Model(backup);assert(m.flushToggle(cycle));m.setLab({tray:false});assert(!m.isFlushing());});
  check(()=>{const m=new Model(backup);m.prefs.trayCalibrated=false;assert.equal(m.flushToggle(cycle),false);});
}
check(()=>{const m=new Model(backup);m.setLab({tray:false,cup:120});assert(m.flushToggle());});
check(()=>{const m=new Model(backup);m.setLab({distance:135});assert.equal(m.flushToggle(),false);});
check(()=>{const m=new Model(backup);assert(m.flushToggle());m.setLab({distance:135});assert(!m.isFlushing());});
check(()=>{const m=new Model(backup);m.go('descale');m.switchBrew(true);assert(m.descale);m.setLab({tray:false});assert(!m.descale);});
check(()=>{const m=new Model(backup);m.cfg.scalesF1=0;assert.equal(m.flushToggle(),false);});
check(()=>{const m=new Model(backup);m.scope='system';m.edit('steamSetPoint');m.scope='profile';m.key('1');m.key('5');m.key('0');m.key('OK');assert.equal(m.cfg.steamSetPoint,150);assert.equal(m.profile().steamSetPoint,undefined);});
check(()=>{const m=new Model(backup);m.scope='system';m.edit('scalesF1');m.key('0');m.key('OK');assert(m.editor.error);assert.equal(m.cfg.scalesF1,3920);});
check(()=>{const m=new Model(backup);m.setLab({cup:120});m.tare();assert.equal(m.weight(),0);assert.equal(m.gross(),290);});
check(()=>{const m=new Model(backup);m.scope='profile';m.edit('shotDose');m.key('2');m.key('0');m.key('OK');assert.equal(m.profile().shotDose,20);assert.equal(m.saved.profiles[0].shotDose,backup.profiles[0].shotDose);m.save();assert.equal(m.saved.profiles[0].shotDose,20);});
check(()=>{const m=new Model(backup);m.switchBrew(true);m.tick(1);assert.equal(m.elapsed,1);assert(m.curve.length);m.switchBrew(false);assert(m.lastShot);});
// Render every page and every editor at the actual framebuffer bounds. A canvas
// stub exercises layout and target generation; visual QA happens in Chrome.
const fakeCtx={font:'12px Arial',fillRect(){},fill(){},drawImage(){},beginPath(){},moveTo(){},lineTo(){},stroke(){},arc(){},fillText(){},measureText(s){return {width:s.length*Number(this.font.match(/(\d+)px/)[1])*0.52};},getImageData(){return {data:new Uint8ClampedArray(320*240*4)};},putImageData(){}};
function element(name){return {style:{},children:[],setAttribute(){},append(...c){this.children.push(...c);},replaceChildren(){this.children=[];},getContext(){return fakeCtx;},onclick:null};}
global.document={createElement:element};global.GaggiuinoTrial=require('./model.js');
vm.runInThisContext(fs.readFileSync(path.join(__dirname,'view.js'),'utf8'));
vm.runInThisContext(fs.readFileSync(path.join(__dirname,'wheels.js'),'utf8'));
vm.runInThisContext(fs.readFileSync(path.join(__dirname,'original-brew.js'),'utf8'));
vm.runInThisContext(fs.readFileSync(path.join(__dirname,'profile-editor.js'),'utf8'));
const pages=['home','menu','profiles','profile','system','boiler','scales','water','tray','cleaning','backflush','descale','steam','hot-water','manual','shot','graph-preview','led'];
for(let i=0;i<10;i++){
  const m=new Model(backup,i),v=new global.GaggiuinoView.View(element('div'),m);
  check(()=>{assert(v.hits.find(h=>h.id==='home-flush'));assert(!v.hits.some(h=>/profile/i.test(h.label)));assert(v.hits.some(h=>h.id==='water-column'&&h.h>h.w));assert(!v.hits.some(h=>h.id==='tray'));assert.equal(v.hits.find(h=>h.id==='home-flush').switchState,false);});
  check(()=>{v.hits.find(h=>h.id==='home-flush').action();v.draw();assert.equal(v.hits.find(h=>h.id==='home-flush').switchState,true);m.setLab({tray:false});assert(!m.isFlushing());m.setLab({tray:true});m.popup=null;});
  for(const page of pages){m.page=page;m.section=0;m.popup=null;m.editor=null;v.draw();check(()=>assert(v.hits.length>0));}
  for(const [scope,groups] of [['profile',profileGroups],['system',systemGroups]])for(const group of Object.keys(groups)){
    m.page='fields';m.scope=scope;m.group=group;for(let section=0;section<Math.ceil(groups[group].length/4);section++){m.section=section;v.draw();check(()=>assert(v.hits.length>0));}
  }
  m.page='water';m.popup={title:'Flush blocked',text:'Hardware scales offline',kind:'blocked'};v.draw();check(()=>assert.equal(v.hits.length,1));m.popup=null;
  m.scope='profile';m.edit('shotDose');v.draw();check(()=>assert(v.hits.some(h=>h.label==='OK')));m.editor=null;m.edit('name');v.draw();check(()=>assert(v.hits.some(h=>h.label==='SPACE')));
}
check(()=>{const m=new Model(backup);assert.equal(m.cfg.profiles[0].name,'Turbo');assert.equal(m.cfg.profiles[1].name,'SOUP');assert.equal(m.cfg.activeProfile,0);assert.equal(m.prefs.fullDistance,30);});
check(()=>{
  const m=new Model(backup);m.scope='profile';m.profile().shotStopOnCustomWeight=0;m.edit('shotPreset');m.key('2');m.key('.');m.key('5');m.key('OK');assert.equal(m.profile().shotPreset,2.5);assert.equal(m.stopTarget(),m.profile().shotDose*2.5);
  m.setStopMode('weight');m.edit('shotStopOnCustomWeight');m.key('4');m.key('2');m.key('OK');m.profile().shotDose=20;assert.equal(m.stopTarget(),42);
  m.setStopMode('ratio');assert.equal(m.stopTarget(),50);m.save();const reboot=new Model(m.saved);assert.equal(reboot.profile().shotPreset,2.5);assert.equal(reboot.stopTarget(),50);
  m.profile().stopOnWeightState=true;m.switchBrew(true);m.elapsed=7;m.extracted=49.9;m.tick(.1);assert(!m.brewing);assert(m.lastShot);
});
check(()=>{global.GAGGIUINO_PROFILE_PROVENANCE={soupPending:true};const m=new Model(backup);m.useProfile(1);m.switchBrew(true);assert(!m.brewing);assert.equal(m.popup.title,'SOUP recipe needed');delete global.GAGGIUINO_PROFILE_PROVENANCE;});
for(let i=0;i<10;i++)check(()=>{
 const m=new Model(backup,i),v=new global.GaggiuinoView.View(element('div'),m);m.go('fields',{scope:'profile',group:'Temperature & dose'});v.draw();
 assert(v.hits.find(h=>h.id==='stop-mode-ratio'));assert(v.hits.find(h=>h.id==='stop-mode-weight'));v.hits.find(h=>h.id==='stop-mode-ratio').action();v.draw();assert(v.hits.find(h=>h.id==='field-shotPreset'));
 v.hits.find(h=>h.id==='stop-mode-weight').action();v.draw();assert(v.hits.find(h=>h.id==='field-shotStopOnCustomWeight'));
 m.go('shot');v.draw();assert.deepEqual(v.hits.map(h=>h.id),['brew-home','brew-tare']);assert(v.hits.every(h=>h.y<40));
});
// Historical manual flow: live slider, physical switch, no scale dependency or profile mutation.
check(()=>{
  const m=new Model(backup),profiles=JSON.stringify(m.cfg.profiles);m.setLab({scales:'offline'});m.go('manual');
  assert(m.manualActive);assert.equal(m.manualFlow,0);m.switchBrew(true);m.tick(2);assert.equal(m.page,'manual');assert.equal(m.pumpFlow,0);assert.equal(m.extracted,0);assert.equal(m.elapsed,2);
  m.setManualFlow(2.31);assert.equal(m.manualFlow,2.3);m.tick(50);assert(m.brewing);assert(m.extracted>m.cfg.profiles[m.cfg.activeProfile].shotDose*2);assert.equal(m.pumpFlow,2.3);
  m.switchBrew(false);const stopped=m.elapsed;m.tick(1);assert.equal(m.pumpFlow,0);assert.equal(m.elapsed,stopped);assert.equal(JSON.stringify(m.cfg.profiles),profiles);
  m.home();assert(!m.manualActive);assert.equal(m.manualFlow,0);m.go('manual');m.setManualFlow(13);assert.equal(m.manualFlow,13);m.setManualFlow(15);assert.equal(m.manualFlow,13);m.setManualFlow(-2);assert.equal(m.manualFlow,0);m.setManualFlow(NaN);assert.equal(m.manualFlow,0);m.back();assert(!m.manualActive);
});
for(let i=0;i<10;i++)check(()=>{
  const m=new Model(backup,i),v=new global.GaggiuinoView.View(element('div'),m);m.go('manual');v.draw();
  const slider=v.hits.find(h=>h.id==='manual-flow-slider');assert.deepEqual(slider.range,{min:0,max:13,step:.1,value:0});slider.action(5.7);v.draw();assert.equal(m.manualFlow,5.7);
  const input=v.overlay.children.find(c=>c.type==='range');assert.equal(input.value,5.7);input.value=6.2;input.oninput();assert.equal(m.manualFlow,6.2);assert.equal(v.overlay.children.find(c=>c.type==='range'),input);
  m.switchBrew(true);assert.equal(m.page,'manual');m.tick(1);v.draw();assert.equal(input.value,6.2);v.hits.find(h=>h.id==='manual-auto').action();assert.equal(m.page,'shot');assert(!m.manualActive);
});
// Selecting remembers only the active index. Editing another profile never selects it.
for(let active=0;active<5;active++)for(let edited=0;edited<5;edited++)check(()=>{
  const m=new Model(backup);m.cfg.offsetTemp=8;m.useProfile(active);
  let reboot=new Model(JSON.parse(JSON.stringify(m.saved)));assert.equal(reboot.cfg.activeProfile,active);assert.equal(reboot.cfg.offsetTemp,backup.offsetTemp);
  m.openProfileEditor(edited);assert.equal(m.editProfile,edited);assert.equal(m.cfg.activeProfile,active);
  m.edit('name');m.key('T');m.key('E');m.key('S');m.key('T');m.key('OK');m.save();
  reboot=new Model(JSON.parse(JSON.stringify(m.saved)));assert.equal(reboot.cfg.activeProfile,active);assert.equal(reboot.cfg.profiles[edited].name,'TEST');
});
for(let i=0;i<10;i++)check(()=>{
  const m=new Model(backup,i),v=new global.GaggiuinoView.View(element('div'),m);m.go('profiles');v.draw();
  v.hits.find(h=>h.id==='edit-profile-1').action();assert.equal(m.page,'profile');assert.equal(m.editProfile,1);assert.equal(m.cfg.activeProfile,backup.activeProfile);
  m.go('profiles');v.draw();v.hits.find(h=>h.id==='select-profile-2').action();assert.equal(m.page,'profiles');assert.equal(m.cfg.activeProfile,2);assert.equal(m.saved.activeProfile,2);
  m.section=1;v.draw();assert(v.hits.find(h=>h.id==='edit-profile-4'));assert(v.hits.find(h=>h.id==='select-profile-4'));
});
// One-screen steps: every field in both mode banks is reachable without paging.
for(let concept=0;concept<10;concept++)check(()=>{
 const m=new Model(backup,concept),v=new global.GaggiuinoView.View(element('div'),m),seen=new Set();m.openProfileEditor(1);v.draw();assert.equal(v.hits.filter(h=>h.id.startsWith('profile-step-')).length,6);
 for(const group of Object.keys(profileGroups)){
  m.page='fields';m.scope='profile';m.group=group;
  for(const mode of [false,true]){
   Object.assign(m.profile(),{preinfusionFlowState:mode,tpType:mode,mfProfileState:mode});
   for(const stop of ['ratio','weight']){m.setStopMode(stop);v.draw();for(const h of v.hits){if(h.fieldKey)seen.add(h.fieldKey);if(h.id.startsWith('field-'))seen.add(h.id.slice(6));if(h.id.startsWith('curve-'))seen.add(h.id.split('-')[1]);}assert(v.hits.some(h=>h.id==='profile-step-prev'));assert(v.hits.some(h=>h.id==='profile-step-next'));assert(!v.hits.some(h=>h.id==='prev'||h.id==='next'));}
  }
 }
 for(const key of Object.values(profileGroups).flat())assert(seen.has(key),'Missing control '+key);
 m.group='Preinfusion';v.draw();v.hits.find(h=>h.id==='profile-step-next').action();assert.equal(m.group,'Soak');assert.equal(m.editProfile,1);assert.equal(m.cfg.activeProfile,0);
});
check(()=>{const m=new Model(backup);m.switchBrew(true);m.tick(.25);assert.equal(m.curve.length,1);assert.equal(m.curve[0].t,.22);m.tick(.75);assert.equal(m.curve.length,4);assert.equal(m.curve[3].t,.88);m.profile().stopOnWeightState=false;m.tick(80);assert.equal(m.curve.length,320);assert(Math.abs(m.curve[319].t-m.curve[0].t-70.18)<1e-8);});
check(()=>{const m=new Model(backup);m.useProfile(1);const p=m.cfg.profiles[1];assert.equal(p.setpoint,86);assert.equal(m.stopTarget(p),51);assert.equal(p.preinfusionFlowTime+p.soakTimeFlow,10);assert.equal(p.mfProfileStart,6);assert.equal(p.mfProfilingPressureRestriction,1);m.switchBrew(true);assert(m.brewing);});
console.log(`PASS: ${checks} checks; 10 concepts; all profile/system fields; tray interlocks; calibration; page and touch bounds.`);
