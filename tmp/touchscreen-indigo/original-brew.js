(function(root){
  'use strict';
  const V=root.GaggiuinoView,assets={};
  if(typeof Image!=='undefined')for(const n of [127,132,133,134,137]){const img=new Image();img.src='assets/'+n+'.png';assets[n]=img;}
  const fmt=n=>Number.isFinite(n)?n.toFixed(1):'--';
  for(const Class of V.wheelClasses){
    const secondary=Class.prototype.secondary;
    Class.prototype.doseSetup=function(){
      const m=this.m,p=m.profile(),byWeight=p.shotStopOnCustomWeight>0,C=this.palette;
      m.scope='profile';if(this.profileStepHeader)this.profileStepHeader('Temperature & dose');else this.top('Temperature & dose',true);
      this.fill(8,39,304,30,C.panel);this.text('Brew temperature',16,48,12,this.dim);this.text(p.setpoint+' °C',303,45,18,this.fg,'right');this.hit('field-setpoint','Brew temperature',8,39,304,30,()=>m.edit('setpoint'));
      this.fill(8,75,304,30,C.panel);this.text('Coffee dose',16,84,12,this.dim);this.text(fmt(p.shotDose)+' g',303,81,18,this.fg,'right');this.hit('field-shotDose','Coffee dose (g)',8,75,304,30,()=>m.edit('shotDose'));
      this.button('Stop on weight: '+(p.stopOnWeightState?'ON':'OFF'),8,111,304,29,()=>m.edit('stopOnWeightState'),'quiet','field-stopOnWeightState');
      this.button('Ratio 1:x',8,146,148,29,()=>m.setStopMode('ratio'),byWeight?'normal':'blue','stop-mode-ratio');
      this.button('Weight (g)',164,146,148,29,()=>m.setStopMode('weight'),byWeight?'blue':'normal','stop-mode-weight');
      this.fill(8,181,304,30,C.panel);this.text(byWeight?'Stop at weight':'Output ratio',16,190,12,this.dim);this.text(byWeight?fmt(p.shotStopOnCustomWeight)+' g':'1:'+Number(p.shotPreset),303,187,18,this.fg,'right');
      this.hit(byWeight?'field-shotStopOnCustomWeight':'field-shotPreset',byWeight?'Output weight in grams':'Output ratio multiplier',8,181,304,30,()=>m.edit(byWeight?'shotStopOnCustomWeight':'shotPreset'));
      this.text(p.stopOnWeightState?`${fmt(p.shotDose)} g in → ${fmt(m.stopTarget(p))} g out`:'Automatic weight stop disabled',160,220,11,this.dim,'center',304);
    };
    Class.prototype.originalBrew=function(){
      const m=this.m,q=this.ctx,BLACK=0,WHITE=65535,PINK=64171,pressureColor=1055,flowColor=65504,weightColor=16051,tempColor=63488;
      this.fill(0,0,320,240,BLACK);
      const img=(n,x,y,w,h)=>{if(assets[n]?.complete&&assets[n].naturalWidth)q.drawImage(assets[n],x,y,w,h);};
      img(127,0,0,40,40);this.hit('brew-home','Home',0,0,40,40,()=>m.home());
      img(132,280,0,40,40);this.hit('brew-tare','Tare',280,0,40,40,()=>m.tare());
      if(!m.brewing||Math.floor(m.elapsed)%2===0)img(m.brewing?133:134,89,10,58,20);
      const seconds=Math.floor(m.elapsed),minutes=String(Math.floor(seconds/60)).padStart(2,'0'),secs=String(seconds%60).padStart(2,'0');
      this.text(minutes,167,5,30,PINK,'center',42);this.text(':',189,2,30,PINK,'center');this.text(secs,211,5,30,PINK,'center',42);
      img(137,0,41,320,144);
      if(!assets[137]?.complete){for(let n=0;n<8;n++)this.line(0,42+n*20,320,42+n*20,16938);}
      const a=m.curve;
      [[v=>v.p,13,893],[v=>v.f,13,flowColor],[v=>v.w,1.3,weightColor],[v=>v.temp??m.lab.temperature,1.3,tempColor]].forEach(([get,scale,c])=>{
        q.strokeStyle=V.rgb565(c);q.lineWidth=1;q.beginPath();a.forEach((v,i)=>{const x=319-Math.round((m.elapsed-v.t)/.220),y=185-Math.max(0,Math.min(143,get(v)*scale));if(x<0||x>319)return;if(i&&319-Math.round((m.elapsed-a[i-1].t)/.220)>=0)q.lineTo(x,y);else q.moveTo(x,y);});q.stroke();
      });
      this.text('Pressure',5,190,12,pressureColor);this.text('Flow(g/s)',83,190,12,flowColor);this.text('Weight(g)',170,190,12,weightColor);this.text('Temp°C',254,190,12,tempColor);
      this.text(fmt(this.pressure),5,207,24,pressureColor,'left',72);this.text(fmt(m.pumpFlow),83,207,24,flowColor,'left',77);this.text(this.weight,170,207,24,weightColor,'left',73);this.text(fmt(m.lab.temperature),252,207,24,tempColor,'left',68);
    };
    Class.prototype.secondary=function(){
      if(this.m.page==='shot'){this.originalBrew();return;}
      if(this.m.page==='fields'&&this.m.group==='Temperature & dose'){this.doseSetup();return;}
      secondary.call(this);
    };
  }
})(typeof globalThis!=='undefined'?globalThis:this);
