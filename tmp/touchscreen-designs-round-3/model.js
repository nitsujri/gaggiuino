(function (root) {
  'use strict';
  const clone = value => JSON.parse(JSON.stringify(value));
  const concepts = [
  {
    "id": "tidal",
    "name": "Tidal",
    "description": "Lagoon proportions · 130 px wheel / 8 px ring · simple Back, Previous, Next labels.",
    "number": 1
  },
  {
    "id": "arctic",
    "name": "Arctic",
    "description": "Glacial blue · 140 px wheel / 10 px ring · clean drawn chevrons.",
    "number": 2
  },
  {
    "id": "celadon",
    "name": "Celadon",
    "description": "Soft green · 148 px wheel / 12 px ring · rounded navigation pills.",
    "number": 3
  },
  {
    "id": "petrol",
    "name": "Petrol",
    "description": "Petrol and citron · 156 px wheel / 14 px ring · lightly outlined navigation.",
    "number": 4
  },
  {
    "id": "abyss",
    "name": "Abyss",
    "description": "Deep blue and cyan · 160 px wheel / 18 px ring · circular arrow controls.",
    "number": 5
  },
  {
    "id": "orchid",
    "name": "Orchid",
    "description": "Smoky violet · 152 px wheel / 20 px ring · understated text and underlines.",
    "number": 6
  },
  {
    "id": "porcelain",
    "name": "Porcelain",
    "description": "Pale mineral and teal · 160 px wheel / 12 px ring · flat navigation with side markers.",
    "number": 7
  },
  {
    "id": "indigo",
    "name": "Indigo",
    "description": "Blue-violet on navy · 144 px wheel / 16 px ring · chevron plus text.",
    "number": 8
  },
  {
    "id": "reed",
    "name": "Reed",
    "description": "Moss and pale gold · 156 px wheel / 22 px ring · soft filled navigation tabs.",
    "number": 9
  },
  {
    "id": "pearl",
    "name": "Pearl",
    "description": "Pearl on slate · 160 px wheel / 24 px ring · compact arrows with short labels.",
    "number": 10
  }
];
  const labels = {
    name:'Profile name', steamSetPoint:'Steam temperature', offsetTemp:'Temperature offset', hpwr:'Heater power',
    mainDivider:'Idle control divider', brewDivider:'Brew control divider', powerLineFrequency:'Mains frequency',
    lcdSleep:'Screen sleep', warmupState:'Warmup', homeOnShotFinish:'Home after shot', brewDeltaState:'Brew delta',
    basketPrefill:'Basket prefill', scalesF1:'Load cell 1 factor', scalesF2:'Load cell 2 factor', pumpFlowAtZero:'Pump zero flow',
    ledState:'LED enabled', ledDisco:'LED disco', ledR:'LED red', ledG:'LED green', ledB:'LED blue',
    preinfusionState:'Preinfusion enabled', preinfusionFlowState:'Use flow control', preinfusionSec:'PI time (s)',
    preinfusionBar:'PI pressure (bar)', preinfusionFlowVol:'PI volume (ml)', preinfusionFlowTime:'PI flow time (s)',
    preinfusionFlowPressureTarget:'PI pressure target', preinfusionPressureFlowTarget:'PI flow target',
    preinfusionFilled:'PI filled volume', preinfusionPressureAbove:'PI pressure above', preinfusionWeightAbove:'PI weight above (g)',
    soakState:'Soak enabled', soakTimePressure:'Pressure soak (s)', soakTimeFlow:'Flow soak (s)', soakKeepPressure:'Soak pressure (bar)',
    soakKeepFlow:'Soak flow (ml/s)', soakBelowPressure:'Stop below (bar)', soakAbovePressure:'Stop above (bar)', soakAboveWeight:'Stop above (g)',
    preinfusionRamp:'PI ramp time (s)', preinfusionRampSlope:'PI ramp curve', tpState:'Transition enabled', tpType:'Use flow transition',
    tpProfilingStart:'Start pressure (bar)', tpProfilingFinish:'End pressure (bar)', tpProfilingHold:'Pressure hold (s)',
    tpProfilingHoldLimit:'Pressure hold limit', tpProfilingSlope:'Pressure ramp (s)', tpProfilingSlopeShape:'Pressure ramp curve',
    tpProfilingFlowRestriction:'Flow limit (ml/s)', tfProfileStart:'Start flow (ml/s)', tfProfileEnd:'End flow (ml/s)',
    tfProfileHold:'Flow hold (s)', tfProfileHoldLimit:'Flow hold limit', tfProfileSlope:'Flow ramp (s)',
    tfProfileSlopeShape:'Flow ramp curve', tfProfilingPressureRestriction:'Pressure limit (bar)',
    profilingState:'Brew profiling enabled', mfProfileState:'Use flow profiling', mpProfilingStart:'Start pressure (bar)',
    mpProfilingFinish:'End pressure (bar)', mpProfilingSlope:'Pressure ramp (s)', mpProfilingSlopeShape:'Pressure ramp curve',
    mpProfilingFlowRestriction:'Flow limit (ml/s)', mfProfileStart:'Start flow (ml/s)', mfProfileEnd:'End flow (ml/s)',
    mfProfileSlope:'Flow ramp (s)', mfProfileSlopeShape:'Flow ramp curve', mfProfilingPressureRestriction:'Pressure limit (bar)',
    setpoint:'Brew temperature', stopOnWeightState:'Stop on weight', shotDose:'Coffee dose (g)',
    shotStopOnCustomWeight:'Custom stop weight (g)', shotPreset:'Shot preset',
    brightness:'Screen brightness', trayMinimum:'Minimum gross load (g)', fullDistance:'Full tank distance (mm)'
  };
  const profileGroups = {
    'Temperature & dose':['setpoint','stopOnWeightState','shotDose','shotStopOnCustomWeight','shotPreset'],
    'Preinfusion':['preinfusionState','preinfusionFlowState','preinfusionSec','preinfusionBar','preinfusionFlowVol','preinfusionFlowTime','preinfusionFlowPressureTarget','preinfusionPressureFlowTarget','preinfusionFilled','preinfusionPressureAbove','preinfusionWeightAbove'],
    'Soak':['soakState','soakTimePressure','soakTimeFlow','soakKeepPressure','soakKeepFlow','soakBelowPressure','soakAbovePressure','soakAboveWeight'],
    'Ramp':['preinfusionRamp','preinfusionRampSlope'],
    'Transition':['tpState','tpType','tpProfilingStart','tpProfilingFinish','tpProfilingHold','tpProfilingHoldLimit','tpProfilingSlope','tpProfilingSlopeShape','tpProfilingFlowRestriction','tfProfileStart','tfProfileEnd','tfProfileHold','tfProfileHoldLimit','tfProfileSlope','tfProfileSlopeShape','tfProfilingPressureRestriction'],
    'Brew profiling':['profilingState','mfProfileState','mpProfilingStart','mpProfilingFinish','mpProfilingSlope','mpProfilingSlopeShape','mpProfilingFlowRestriction','mfProfileStart','mfProfileEnd','mfProfileSlope','mfProfileSlopeShape','mfProfilingPressureRestriction']
  };
  const systemGroups = {
    'Boiler':['steamSetPoint','offsetTemp','hpwr','mainDivider','brewDivider'],
    'Scales & pump':['scalesF1','scalesF2','pumpFlowAtZero'],
    'Screen & warmup':['lcdSleep','warmupState','brightness'],
    'Brew preferences':['homeOnShotFinish','brewDeltaState','basketPrefill'],
    'Power & lighting':['powerLineFrequency','ledState','ledDisco','ledR','ledG','ledB']
  };
  // Preserve the installed low-water boundary: >125 mm is 9%, below MIN_WATER_LVL=10.
  function waterPercent(distance, fullDistance, valid=true) {
    if (!valid || !Number.isFinite(distance) || distance <= 0) return null;
    if (distance > 125) return 9;
    if (distance <= fullDistance) return 100;
    return Math.max(10, Math.min(100, Math.round(10 + 90 * (125-distance)/(125-fullDistance))));
  }
  class Model {
    constructor(backup, concept=0) {
      this.original = clone(backup); this.cfg = clone(backup); this.concept=concept;
      this.saved=clone(backup); this.prefs={brightness:90,fullDistance:28,trayMinimum:100,trayCalibrated:true};
      this.savedPrefs=clone(this.prefs); this.lab={tray:true,cup:0,scales:'ok',distance:62,tofValid:true,temperature:74.6};
      this.page='home'; this.history=[]; this.section=0; this.editProfile=this.cfg.activeProfile;
      this.flush=false; this.backflush=false; this.descale=false; this.steam=false; this.hotWater=false;
      this.brewing=false; this.physicalSwitch=false; this.scalesView=true; this.manualFlow=0; this.manualActive=false;
      this.elapsed=0; this.flushElapsed=0; this.descaleElapsed=0; this.backflushElapsed=0;
      this.extracted=0; this.tareRaw=this.rawLoad(); this.curve=[]; this.lastShot=null;
      this.popup=null; this.editor=null; this.message='Ready'; this.group=null; this.scope='profile';
    }
    rawLoad() {const mass=(this.lab.tray?170:0)+this.lab.cup+this.extracted;return [mass*3920/2,mass*4210/2];}
    gross() {
      const raw=this.rawLoad(),f=[this.cfg.scalesF1,this.cfg.scalesF2];
      return f.every(x=>Number.isFinite(x)&&x!==0)?raw[0]/f[0]+raw[1]/f[1]:NaN;
    }
    weight() {const raw=this.rawLoad();return (raw[0]-this.tareRaw[0])/this.cfg.scalesF1+(raw[1]-this.tareRaw[1])/this.cfg.scalesF2;}
    water() {return waterPercent(this.lab.distance,this.prefs.fullDistance,this.lab.tofValid);}
    trayReason() {
      if (!this.prefs.trayCalibrated) return 'Calibrate tray guard';
      if (this.lab.scales==='offline') return 'Hardware scales offline';
      if (this.lab.scales==='stale') return 'Scale reading is stale';
      if (this.lab.scales==='invalid'||!Number.isFinite(this.gross())) return 'Scale reading invalid';
      if (this.gross()<this.prefs.trayMinimum) return 'Place tray or container';
      return null;
    }
    flushReason() {return this.trayReason() || (this.water()!==null && this.water()<10?'Fill the water tank':null);}
    isFlushing() {return this.flush||this.backflush;}
    stopFlush(message='Flush stopped') {this.flush=false;this.backflush=false;this.flushElapsed=0;this.backflushElapsed=0;this.message=message;}
    enforce() {const reason=this.flushReason();if ((this.isFlushing()||this.descale)&&reason){this.stopFlush(reason);this.descale=false;this.popup={title:'Flush stopped',text:reason,kind:'blocked'};}}
    setLab(changes) {Object.assign(this.lab,changes);this.enforce();}
    setPage(page) {const manual=page==='manual';if(manual!==this.manualActive)this.manualFlow=0;this.manualActive=manual;this.page=page;}
    setManualFlow(value) {if(Number.isFinite(value))this.manualFlow=Math.round(Math.max(0,Math.min(13,value))*10)/10;}
    get pumpFlow() {return !this.brewing?0:this.manualActive?this.manualFlow:this.elapsed<6?.2:1.7;}
    go(page, extra={}) {if(this.page!==page)this.history.push({page:this.page,section:this.section,group:this.group,scope:this.scope});this.setPage(page);this.section=0;Object.assign(this,extra);this.editor=null;}
    back() {const prior=this.history.pop();if(prior)Object.assign(this,prior);else this.page='home';this.setPage(this.page);this.editor=null;}
    home() {this.setPage('home');this.history=[];this.editor=null;}
    flushToggle(backflush=false) {
      if(this.isFlushing()){this.stopFlush();return false;}
      const reason=this.flushReason();
      if(reason){this.popup={title:'Flush blocked',text:reason,kind:'blocked'};return false;}
      if(this.brewing||this.steam||this.hotWater||this.descale){this.popup={title:'Machine busy',text:'Stop the active mode first.'};return false;}
      this.flush=!backflush;this.backflush=backflush;this.flushElapsed=0;this.backflushElapsed=0;
      this.message=backflush?'Backflush running':'Flushing group';return true;
    }
    tare() {if(this.lab.scales!=='ok'){this.popup={title:'Tare unavailable',text:'Check hardware scales.'};return;}this.tareRaw=this.rawLoad();this.message='Scales tared';}
    useProfile(i) {if(!Number.isInteger(i)||!this.cfg.profiles[i])return;this.cfg.activeProfile=i;this.saved.activeProfile=i;this.message='Profile selected';}
    openProfileEditor(i) {if(!Number.isInteger(i)||!this.cfg.profiles[i])return;this.go('profile',{editProfile:i,scope:'profile'});}
    profile() {return this.cfg.profiles[this.editProfile];}
    fieldKeys() {return this.scope==='profile'?profileGroups[this.group]||[]:systemGroups[this.group]||[];}
    value(key) {return key in this.prefs?this.prefs[key]:(this.scope==='profile'?this.profile()[key]:this.cfg[key]);}
    edit(key) {
      const value=this.value(key);
      if(typeof value==='boolean'){this.setValue(key,!value);return;}
      this.editor={key,label:labels[key]||key,value:String(value),string:typeof value==='string',fresh:true,target:key in this.prefs?this.prefs:(this.scope==='profile'?this.profile():this.cfg)};
    }
    setValue(key,value) {if(key in this.prefs)this.prefs[key]=value;else if(this.scope==='profile')this.profile()[key]=value;else this.cfg[key]=value;this.enforce();}
    key(key) {
      if(!this.editor)return;
      const e=this.editor;
      if(key==='DEL'){e.value=e.value.slice(0,-1);e.fresh=false;return;}
      if(key==='CLEAR'){e.value='';e.fresh=false;return;}
      if(key==='CANCEL'){this.editor=null;return;}
      if(key==='OK'){
        const value=e.string?e.value:Number(e.value);
        let error=null;
        if(e.string&&(!value.trim()||value.length>24))error='Use 1 to 24 characters.';
        if(!e.string&&(!Number.isFinite(value)||e.value===''))error='Enter a valid number.';
        if(['scalesF1','scalesF2'].includes(e.key)&&(value===0||Math.abs(value)>20000))error='Factor must be nonzero, +/-20000.';
        if(e.key==='fullDistance'&&(value<1||value>=125))error='Full distance must be 1-124 mm.';
        if(e.key==='trayMinimum'&&value<=0)error='Minimum weight must be positive.';
        if(['setpoint','steamSetPoint'].includes(e.key)&&(value<1||value>165))error='Temperature must be 1-165 C.';
        if(e.key==='pumpFlowAtZero'&&(value<.21||value>.31))error='Use 0.2100 to 0.3100.';
        if(['brightness','ledR','ledG','ledB'].includes(e.key)&&(value<0||value>(e.key==='brightness'?100:255)))error='Value outside allowed range.';
        if(e.key==='powerLineFrequency'&&![50,60].includes(value))error='Choose 50 or 60 Hz.';
        if(error){e.error=error;return;}
        e.target[e.key]=value;this.enforce();this.editor=null;this.message='Changed; tap Save to retain';return;
      }
      if(e.fresh){e.value='';e.fresh=false;}
      if(e.string)e.value=(e.value+(key==='SPACE'?' ':key)).slice(0,24);
      else if(key==='-')e.value=e.value.startsWith('-')?e.value.slice(1):'-'+e.value;
      else if(key==='.'&&!e.value.includes('.'))e.value+='.';
      else if(/^\d$/.test(key))e.value+=key;
      e.error=null;
    }
    save() {this.saved=clone(this.cfg);this.savedPrefs=clone(this.prefs);this.message='Settings saved';this.popup={title:'Saved',text:'Settings saved successfully.'};}
    setFull() {
      if(!this.lab.tofValid||this.lab.distance<=0||this.lab.distance>=125){this.popup={title:'Cannot calibrate',text:'Need a valid full-tank reading.'};return false;}
      this.prefs.fullDistance=this.lab.distance;this.message='Full point captured; tap Save';return true;
    }
    switchBrew(on) {
      this.physicalSwitch=on;
      if(!on){if(this.brewing)this.lastShot={seconds:this.elapsed,weight:this.extracted};this.brewing=false;this.hotWater=false;this.descale=false;this.stopFlush('Ready');return;}
      if(this.page==='backflush'){this.flushToggle(true);return;}
      if(this.page==='descale'){const reason=this.flushReason();if(reason){this.popup={title:'Descale blocked',text:reason,kind:'blocked'};return;}this.descale=true;this.descaleElapsed=0;return;}
      if(this.page==='hot-water'){this.hotWater=true;return;}
      if(this.water()!==null&&this.water()<10){this.popup={title:'Low water',text:'Fill the water tank.'};return;}
      if(this.isFlushing()||this.steam)return;
      this.elapsed=0;this.extracted=0;this.curve=[];this.brewing=true;this.tareRaw=this.rawLoad();if(!this.manualActive)this.go('shot');
    }
    tick(dt) {
      this.enforce();
      if(this.flush)this.flushElapsed+=dt;
      if(this.backflush){this.backflushElapsed+=dt;if(this.backflushElapsed>=55)this.stopFlush('Backflush complete');}
      if(this.descale){this.descaleElapsed+=dt;if(this.descaleElapsed>=184)this.descale=false;}
      if(this.brewing){
        this.elapsed+=dt;
        const flow=this.pumpFlow;
        this.extracted+=flow*dt;this.curve.push({t:this.elapsed,p:this.elapsed<6?this.elapsed/2:9,f:flow,w:this.extracted});
        if(this.curve.length>450)this.curve.shift();
        const p=this.cfg.profiles[this.cfg.activeProfile],target=p.shotStopOnCustomWeight>0?p.shotStopOnCustomWeight:p.shotDose*2;
        if(!this.manualActive&&p.stopOnWeightState&&this.extracted>=target){this.lastShot={seconds:this.elapsed,weight:this.extracted};this.brewing=false;this.message='Shot complete';if(this.cfg.homeOnShotFinish)this.home();}
      }
    }
  }
  const api={Model,concepts,labels,profileGroups,systemGroups,waterPercent,clone};
  root.GaggiuinoTrial=api;
  if(typeof module!=='undefined'&&module.exports)module.exports=api;
})(typeof globalThis!=='undefined'?globalThis:this);
