(function(root){
  'use strict';
  const V=root.GaggiuinoView,T=root.GaggiuinoTrial,steps=Object.keys(T.profileGroups);
  const titles=['Temp & dose','Preinfusion','Soak','Ramp','Transition','Brew profile'];
  const curves=['In/out','Ease in','Ease out','Linear','Instant'];
  for(const Class of V.wheelClasses){
    const previous=Class.prototype.secondary;
    Class.prototype.profileStepHeader=function(group){
      const m=this.m,i=steps.indexOf(group),C=this.palette;
      this.button('All',4,3,38,29,()=>{m.page='profile';m.section=0;},'quiet','profile-overview');
      const arrow=(id,label,x,d)=>{
        this.fill(x,3,29,29,C.panel);const q=this.ctx;q.font='20px Arial';q.textAlign='center';q.textBaseline='middle';q.fillStyle=V.rgb565(this.fg);q.fillText(label,x+14.5,17.5);
        this.hit(id,d<0?'Previous step':'Next step',x,3,29,29,()=>{m.group=steps[(i+d+steps.length)%steps.length];m.section=0;});
      };
      arrow('profile-step-prev','‹',46,-1);this.text(titles[i],155,10,14,this.fg,'center',150);arrow('profile-step-next','›',235,1);
      this.button('Save',269,3,47,29,()=>m.save(),'quiet','profile-save');this.line(4,35,316,35,C.slate);
    };
    Class.prototype.profileOverview=function(){
      const m=this.m,p=m.profile(),C=this.palette;m.scope='profile';this.top(p.name,true);
      const summary=[p.setpoint+' °C · '+p.shotDose+' g',p.preinfusionState?(p.preinfusionFlowState?'Flow':'Pressure'):'Off',p.soakState?'Enabled':'Off',p.preinfusionRamp+' s',p.tpState?(p.tpType?'Flow':'Pressure'):'Off',p.profilingState?(p.mfProfileState?'Flow':'Pressure'):'9 bar'];
      steps.forEach((group,i)=>{const x=8+(i%2)*156,y=41+Math.floor(i/2)*51;this.fill(x,y,148,45,C.panel);this.text(titles[i],x+9,y+6,14,this.fg,'left',130);this.text(summary[i],x+9,y+25,11,this.dim,'left',130);this.hit('profile-step-'+i,group,x,y,148,45,()=>m.go('fields',{scope:'profile',group}));});
      this.button('Rename',8,200,96,33,()=>m.edit('name'),'quiet','profile-rename');this.button('Preview',112,200,96,33,()=>m.go('graph-preview'),'quiet','profile-preview');
      this.button('Defaults',216,200,96,33,()=>{m.popup={title:'Load default?',text:'Replace this profile with its starting recipe.',confirm:()=>{m.cfg.profiles[m.editProfile]=T.clone(m.original.profiles[m.editProfile]);}};},'quiet','profile-defaults');
    };
    Class.prototype.profileStep=function(){
      const m=this.m,p=m.profile(),g=m.group,C=this.palette;m.scope='profile';this.profileStepHeader(g);
      const cell=(key,label,i)=>{const x=8+i%2*156,y=76+Math.floor(i/2)*41;this.fill(x,y,148,36,C.panel);this.text(label,x+6,y+3,10,this.dim,'left',136);const v=p[key];this.text(typeof v==='boolean'?(v?'ON':'OFF'):Number(v).toString(),x+6,y+16,17,this.fg,'left',136);this.hit('field-'+key,T.labels[key]||label,x,y,148,36,()=>m.edit(key));};
      const curve=(key,y=204)=>curves.forEach((name,i)=>this.button(name,8+i*62,y,56,29,()=>m.setValue(key,i),p[key]===i?'blue':'normal','curve-'+key+'-'+i));
      const mode=(enabled,key)=>{this.button(p[enabled]?'ON':'OFF',8,41,68,28,()=>m.edit(enabled),p[enabled]?'blue':'normal','field-'+enabled);this.button('Pressure',84,41,110,28,()=>m.setValue(key,false),!p[key]?'blue':'normal','mode-pressure');this.button('Flow',202,41,110,28,()=>m.setValue(key,true),p[key]?'blue':'normal','mode-flow');this.hits[this.hits.length-1].fieldKey=key;};
      if(g==='Preinfusion'){
        mode('preinfusionState','preinfusionFlowState');
        [[p.preinfusionFlowState?'preinfusionFlowTime':'preinfusionSec','Time (s)'],[p.preinfusionFlowState?'preinfusionFlowVol':'preinfusionPressureFlowTarget','Flow (ml/s)'],[p.preinfusionFlowState?'preinfusionFlowPressureTarget':'preinfusionBar','Pressure (bar)'],['preinfusionFilled','Pumped exit (ml)'],['preinfusionPressureAbove','Exit at pressure'],['preinfusionWeightAbove','Weight exit (g)']].forEach(([k,l],i)=>cell(k,l,i));
        this.text('0 disables numeric exit; first condition wins',160,215,11,this.dim,'center',304);
      } else if(g==='Soak'){
        this.button('Soak '+(p.soakState?'ON':'OFF'),8,41,110,28,()=>m.edit('soakState'),p.soakState?'blue':'normal','field-soakState');this.text((p.preinfusionFlowState?'Flow':'Pressure')+' PI time bank',215,49,12,this.dim,'center',184);
        [[p.preinfusionFlowState?'soakTimeFlow':'soakTimePressure','Time (s)'],['soakKeepPressure','Hold pressure (bar)'],['soakKeepFlow','Hold flow (ml/s)'],['soakBelowPressure','Exit below (bar)'],['soakAbovePressure','Exit above (bar)'],['soakAboveWeight','Weight exit (g)']].forEach(([k,l],i)=>cell(k,l,i));
        this.text('0 disables hold / exit conditions',160,215,11,this.dim,'center');
      } else if(g==='Ramp'){
        this.text('Preinfusion → extraction ramp',160,49,13,this.dim,'center');cell('preinfusionRamp','Duration (s)',0);this.text('Curve',8,177,12,this.dim);curve('preinfusionRampSlope');
      } else if(g==='Transition'){
        mode('tpState','tpType');const keys=p.tpType?['tfProfileStart','tfProfileEnd','tfProfileHold','tfProfileHoldLimit','tfProfileSlope','tfProfilingPressureRestriction']:['tpProfilingStart','tpProfilingFinish','tpProfilingHold','tpProfilingHoldLimit','tpProfilingSlope','tpProfilingFlowRestriction'];
        const unit=p.tpType?'ml/s':'bar',limit=p.tpType?'bar':'ml/s';keys.forEach((k,i)=>cell(k,['Start ('+unit+')','End ('+unit+')','Hold (s)','Hold limit ('+limit+')','Ramp time (s)','Ramp limit ('+limit+')'][i],i));curve(p.tpType?'tfProfileSlopeShape':'tpProfilingSlopeShape');
      } else if(g==='Brew profiling'){
        mode('profilingState','mfProfileState');const keys=p.mfProfileState?['mfProfileStart','mfProfileEnd','mfProfileSlope','mfProfilingPressureRestriction']:['mpProfilingStart','mpProfilingFinish','mpProfilingSlope','mpProfilingFlowRestriction'];const unit=p.mfProfileState?'ml/s':'bar',limit=p.mfProfileState?'bar':'ml/s';keys.forEach((k,i)=>cell(k,['Start ('+unit+')','End ('+unit+')','Ramp time (s)','Limit ('+limit+')'][i],i));curve(p.mfProfileState?'mfProfileSlopeShape':'mpProfilingSlopeShape');
      }
    };
    Class.prototype.secondary=function(){
      if(this.m.page==='profile'){this.profileOverview();return;}
      if(this.m.page==='fields'&&this.m.scope==='profile'&&this.m.group!=='Temperature & dose'){this.profileStep();return;}
      previous.call(this);
    };
  }
})(typeof globalThis!=='undefined'?globalThis:this);
