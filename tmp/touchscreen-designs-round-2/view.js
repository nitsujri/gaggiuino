(function(root){
  'use strict';
  const T=root.GaggiuinoTrial;
  const BASE_COLORS={black:0,panel:8518,slate:16938,blue:22142,coral:64171,muted:48664,white:65535,red:63488,yellow:65504,green:1024,grey:50712};
  function rgb565(n){const r=n>>11&31,g=n>>5&63,b=n&31;return `rgb(${r<<3|r>>2},${g<<2|g>>4},${b<<3|b>>2})`;}
  const fmt=n=>Number.isFinite(n)?Math.abs(n)<.05?'0.0':n.toFixed(1):'--';
  const short=n=>typeof n==='boolean'?(n?'ON':'OFF'):typeof n==='number'?(Number.isInteger(n)?String(n):Number(n.toFixed(4)).toString()):String(n);
  function createView(palette={}) {
  const C={...BASE_COLORS,...palette};
  class View {
    constructor(container,model){
      this.el=container;this.m=model;this.palette=C;this.canvas=document.createElement('canvas');this.canvas.width=320;this.canvas.height=240;
      this.canvas.setAttribute('aria-label','320 by 240 Gaggiuino touchscreen');this.canvas.setAttribute('role','img');
      this.overlay=document.createElement('div');this.overlay.className='touch-targets';this.el.append(this.canvas,this.overlay);
      this.ctx=this.canvas.getContext('2d',{willReadFrequently:true});this.signature='';this.hits=[];this.draw();
    }
    fill(x,y,w,h,c){this.ctx.fillStyle=rgb565(c);this.ctx.fillRect(x,y,w,h);}
    line(x,y,x2,y2,c=C.slate,width=1){const q=this.ctx;q.strokeStyle=rgb565(c);q.lineWidth=width;q.beginPath();q.moveTo(x+.5,y+.5);q.lineTo(x2+.5,y2+.5);q.stroke();}
    text(s,x,y,size=14,color=this.fg,align='left',maxWidth=320){
      const q=this.ctx;q.font=`${size>=24?'600':'400'} ${size}px Arial, sans-serif`;q.fillStyle=rgb565(color);q.textAlign=align;q.textBaseline='top';s=String(s);
      if(q.measureText(s).width>maxWidth){while(s.length&&q.measureText(s+'...').width>maxWidth)s=s.slice(0,-1);s=s?s+'...':'';}q.fillText(s,x,y);
    }
    hit(id,label,x,y,w,h,action,disabled=false){
      if(x<0||y<0||x+w>320||y+h>240)throw Error(`Touch target outside display: ${id}`);
      this.hits.push({id,label,x,y,w,h,action,disabled});
    }
    button(label,x,y,w,h,action,style='normal',id=label){
      const light=C.light===undefined?this.m.concept===3:C.light;
      const styles={normal:[light?C.muted:C.panel,this.fg],primary:[C.coral,C.black],blue:[C.blue,C.black],danger:[C.red,C.white],quiet:[this.bg,this.fg],disabled:[light?C.grey:C.panel,light?C.slate:C.muted]};
      const [bg,fg]=styles[style]||styles.normal;this.fill(x,y,w,h,bg);
      if(w<100&&label==='STOP FLUSH'){
        this.text('STOP',x+w/2,y+h/2-13,12,fg,'center',w-6);this.text('FLUSH',x+w/2,y+h/2+1,12,fg,'center',w-6);
      }else this.text(label,x+w/2,y+(h-14)/2,w<60?12:14,fg,'center',w-8);
      this.hit(id,label,x,y,w,h,action,style==='disabled');
    }
    metric(label,value,unit,x,y,w,h,action,size=32,color=this.fg){
      this.fill(x,y,w,h,this.card);this.text(label,x+8,y+6,12,this.dim,'left',w-16);
      this.text(value,x+8,y+23,size,color,'left',w-40);this.text(unit,x+w-9,y+h-23,12,this.dim,'right',35);
      if(action)this.hit(label,label,x,y,w,h,action);
    }
    get temp(){return this.m.steam?this.m.cfg.steamSetPoint:this.m.cfg.profiles[this.m.cfg.activeProfile].setpoint;}
    get pressure(){return this.m.brewing?(this.m.manualActive?Math.min(9,this.m.manualFlow*2.2):this.m.elapsed<6?this.m.elapsed/2:9):this.m.isFlushing()?1.2:.3;}
    get weight(){return !this.m.scalesView?'OFF':this.m.lab.scales==='ok'?fmt(this.m.weight()):'--';}
    get flushLabel(){return this.m.isFlushing()?'STOP FLUSH':'FLUSH';}
    get status(){return this.m.isFlushing()?'FLUSHING':this.m.brewing?'BREWING':this.m.steam?'STEAM':this.m.cfg.warmupState?'WARMUP':'READY';}
    tank(x,y,w=80){const p=this.m.water(),label=w<80?'H2O':'WATER';this.text(p===null?label+' --':`${label} ${p}%`,x,y,12,p!==null&&p<10?C.red:this.dim,'left',w);this.hit('tank','Water tank calibration',x,y-2,w,21,()=>this.m.go('water'));}
    tray(x,y,w=200){const reason=this.m.trayReason();this.text(reason?'TRAY: CHECK':'TRAY: READY',x,y,12,reason?C.coral:this.dim,'left',w);this.hit('tray','Tray guard',x,y-2,w,23,()=>this.m.go('tray'));}
    waterBar(x,y,w,h=5){const p=this.m.water();this.fill(x,y,w,h,this.card);if(p!==null)this.fill(x,y,Math.round(w*p/100),h,p<10?C.red:C.blue);}
    mini(label,value,x,y,w=90){this.text(label,x,y,11,this.dim,'left',w);this.text(value,x,y+15,19,this.fg,'left',w);}
    flush(x,y,w,h){this.button(this.flushLabel,x,y,w,h,()=>this.m.flushToggle(),this.m.isFlushing()?'danger':'primary','home-flush');}
    menu(x,y,w,h){this.button('MENU',x,y,w,h,()=>this.m.go('menu'),'normal','menu');}
    tare(x,y,w,h){this.button('TARE',x,y,w,h,()=>this.m.tare(),'normal','tare');}
    commonHomeTitle(){this.text('GAGGIUINO',8,7,12,this.dim);this.text(this.status,312,7,12,this.m.isFlushing()?C.coral:this.dim,'right');}
    home(){
      const m=this.m,toTemp=()=>m.go('boiler'),toScale=()=>m.go('scales'),toShot=()=>m.go('shot');
      switch(m.concept){
        case 0:
          this.commonHomeTitle();
          this.metric('TEMPERATURE',this.temp,'C',8,30,148,76,toTemp,38,C.coral);
          this.metric('WEIGHT',this.weight,'g',164,30,148,76,toScale,38);
          this.mini('PRESSURE',fmt(this.pressure)+' bar',12,119);this.mini('SHOT',fmt(m.elapsed)+' s',120,119);this.tank(230,119,82);
          this.waterBar(230,141,78,7);this.tray(12,163,210);this.text('Shot >',308,164,12,this.dim,'right');this.hit('shot','Shot monitor',244,157,68,29,toShot);
          this.flush(8,194,160,38);this.tare(176,194,68,38);this.menu(252,194,60,38);break;
        case 1:
          this.fill(0,0,94,240,C.panel);this.text('GAGGI',10,13,15,C.white);this.text('UINO',10,32,15,C.white);
          this.flush(6,65,82,92);this.tare(6,165,82,30);this.menu(6,202,82,32);
          this.metric('BOILER',this.temp,'C',102,8,210,75,toTemp,43,C.coral);
          this.metric('WEIGHT',this.weight,'g',102,91,124,68,toScale,31);
          this.text(fmt(this.pressure),268,101,28,this.fg,'center');this.text('bar',268,134,12,this.dim,'center');this.hit('pressure','Shot monitor',234,91,78,68,toShot);
          this.tank(106,174,150);this.waterBar(106,198,200,6);this.tray(106,218,200);break;
        case 2:
          this.commonHomeTitle();this.dial(80,102,62,this.pressure,12,C.coral);this.text('PRESSURE',80,88,11,this.dim,'center');this.text(fmt(this.pressure),80,104,29,this.fg,'center');this.text('bar',80,135,12,this.dim,'center');this.hit('dial','Shot monitor',10,31,140,145,toShot);
          this.metric('BOILER',this.temp,'C',163,32,149,66,toTemp,30,C.coral);this.metric('SCALES',this.weight,'g',163,106,149,66,toScale,30);
          this.tank(12,176,105);this.tray(153,177,157);this.flush(8,204,160,30);this.tare(176,204,64,30);this.menu(248,204,64,30);break;
        case 3:
          this.text('SCALES',12,10,13,C.slate);this.tank(222,10,95);this.text(this.weight,158,39,64,C.black,'center',290);this.text('GRAMS',158,107,12,C.slate,'center');this.hit('weight','Scales',8,35,304,91,toScale);
          this.line(12,133,308,133,C.grey);this.text(this.temp+' C',12,146,23,C.black);this.hit('temp','Boiler settings',8,138,95,43,toTemp);this.text(fmt(this.pressure)+' bar',124,146,20,C.black);this.hit('shot','Shot monitor',110,138,100,43,toShot);this.tray(224,146,94);
          this.flush(8,194,160,38);this.tare(176,194,64,38);this.menu(248,194,64,38);break;
        case 4:
          this.commonHomeTitle();this.text(this.temp,139,35,70,C.white,'center',235);this.text('C',235,57,23,C.coral);this.text('BOILER TEMPERATURE',160,113,12,C.muted,'center');this.hit('temperature','Boiler settings',8,29,304,102,toTemp);
          this.text(this.weight+' g',12,143,24,C.blue);this.hit('weight','Scales',8,137,132,40,toScale);this.text(fmt(this.pressure)+' bar',169,146,19,C.white);this.hit('pressure','Shot monitor',155,137,86,40,toShot);this.tank(250,146,68);
          this.flush(8,190,166,42);this.tare(182,190,60,42);this.menu(250,190,62,42);break;
        case 5:
          this.fill(0,0,58,240,C.panel);this.text('G',29,10,25,C.coral,'center');this.button('HOME',3,46,52,33,()=>m.home(),'quiet');this.button('SHOT',3,88,52,33,toShot,'quiet');this.flush(3,130,52,53);this.menu(3,195,52,39);
          this.metric('BOILER',this.temp,'C',66,8,246,72,toTemp,41,C.coral);this.metric('WEIGHT',this.weight,'g',66,88,157,72,toScale,36);this.button('TARE',231,88,81,72,()=>m.tare(),'blue');
          this.text(fmt(this.pressure)+' bar',70,174,23,this.fg);this.tank(215,179,97);this.tray(70,213,238);this.waterBar(70,199,238,4);break;
        case 6:
          this.text('GAGGIUINO / '+this.status,8,7,12,this.dim);this.metric('BOILER',this.temp,'C',8,30,148,65,toTemp,29,C.coral);this.metric('PRESSURE',fmt(this.pressure),'bar',164,30,148,65,toShot,29,C.blue);
          this.metric('WEIGHT',this.weight,'g',8,103,148,65,toScale,29);this.metric('SHOT TIME',fmt(m.elapsed),'s',164,103,148,65,toShot,29);
          this.tank(12,177,110);this.tray(165,177,150);this.flush(8,204,146,30);this.tare(162,204,70,30);this.menu(240,204,72,30);break;
        case 7:
          this.text('GAGGIUINO',8,7,12,this.dim);this.tank(221,7,92);this.text(this.temp+' C',9,32,31,C.coral);this.hit('temp','Boiler settings',8,29,125,45,toTemp);this.text(this.weight+' g',309,32,31,this.fg,'right',158);this.hit('weight','Scales',150,29,162,45,toScale);
          this.graph(8,83,304,84);this.hit('graph','Shot graph',8,83,304,84,toShot);this.tray(12,176,210);this.text(fmt(this.pressure)+' bar',307,175,14,C.blue,'right');this.flush(8,202,160,32);this.tare(176,202,64,32);this.menu(248,202,64,32);break;
        case 8:
          this.fill(0,0,320,28,C.coral);this.text('GAGGIUINO',10,7,13,C.black);this.text(this.status,310,7,12,C.black,'right');
          this.metric('TEMPERATURE',this.temp,'C',8,36,148,89,toTemp,48,C.coral);this.metric('NET WEIGHT',this.weight,'g',164,36,148,89,toScale,43);
          this.text(fmt(this.pressure)+' bar',12,141,23,this.fg);this.hit('pressure','Shot monitor',8,134,105,40,toShot);this.tank(126,146,100);this.tray(232,146,80);this.waterBar(12,177,296,4);
          this.flush(0,197,128,43);this.tare(130,197,62,43);this.button('SHOT',194,197,62,43,toShot);this.menu(258,197,62,43);break;
        case 9:
          this.text('GAGGIUINO',10,9,12,C.blue);this.tank(227,9,85);this.text(this.temp+' C',10,38,47,C.white);this.hit('temp','Boiler settings',8,35,152,58,toTemp);this.text(this.weight+' g',310,45,34,C.blue,'right',148);this.hit('weight','Scales',170,35,142,58,toScale);
          this.line(10,103,310,103,C.slate);this.tray(12,113,200);this.text(fmt(this.pressure)+' bar',310,112,16,C.muted,'right');this.hit('shot','Shot monitor',230,105,82,31,toShot);
          this.flush(8,143,304,48);this.tare(8,200,148,32);this.menu(164,200,148,32);break;
      }
      if(m.water()!==null&&m.water()<10){this.fill(0,0,320,25,C.red);this.text('LOW WATER - FILL TANK',160,5,13,C.white,'center');}
    }
    top(title,save=false){this.button('<',4,3,33,26,()=>this.m.back(),'quiet','back');this.text(title,44,8,14,this.fg,'left',195);this.button(save?'SAVE':'HOME',252,3,64,26,()=>save?this.m.save():this.m.home(),save?'blue':'normal','top-action');this.line(4,32,316,32,this.card);}
    pager(total,save=false){const m=this.m;this.button('< PREV',8,201,88,33,()=>{m.section=Math.max(0,m.section-1);},m.section===0?'disabled':'normal','previous');this.text(`${m.section+1} / ${Math.max(1,total)}`,160,212,12,this.dim,'center');this.button('NEXT >',224,201,88,33,()=>{m.section=Math.min(total-1,m.section+1);},m.section>=total-1?'disabled':'normal','next');}
    list(title,items,save=false){
      this.top(title,save);const m=this.m,pages=Math.max(1,Math.ceil(items.length/4));m.section=Math.min(m.section,pages-1);
      items.slice(m.section*4,m.section*4+4).forEach((item,i)=>{this.button(item[0],8,39+i*39,304,34,item[1],item[2]||'normal','row-'+i);});this.pager(pages);
    }
    fields(title,keys){
      this.top(title,true);const m=this.m,pages=Math.max(1,Math.ceil(keys.length/4));m.section=Math.min(m.section,pages-1);
      keys.slice(m.section*4,m.section*4+4).forEach((key,i)=>{const y=39+i*39;this.fill(8,y,304,34,this.card);this.text(T.labels[key]||key,16,y+10,12,this.dim,'left',210);this.text(short(m.value(key)),303,y+8,15,this.fg,'right',85);this.hit('field-'+key,T.labels[key]||key,8,y,304,34,()=>m.edit(key));});this.pager(pages);
    }
    secondary(){
      const m=this.m;
      switch(m.page){
        case 'menu':this.list('Menu',[
          ['Brew setup',()=>m.go('profile',{editProfile:m.cfg.activeProfile})],['Profiles',()=>m.go('profiles')],['Shot monitor',()=>m.go('shot')],['Manual flow',()=>m.go('manual')],
          ['Temperature & machine',()=>m.go('system')],['Water tank / set full',()=>m.go('water')],['Scales & tray guard',()=>m.go('scales')],['Cleaning',()=>m.go('cleaning')],
          ['Steam',()=>m.go('steam')],['Hot water',()=>m.go('hot-water')],['Lighting',()=>m.go('led')],['More brew settings',()=>m.go('fields',{scope:'system',group:'Brew preferences'})]
        ]);break;
        case 'profiles':{
          this.top('Profiles');const pages=Math.ceil(m.cfg.profiles.length/4);m.section=Math.min(m.section,pages-1);
          m.cfg.profiles.slice(m.section*4,m.section*4+4).forEach((p,row)=>{
            const i=m.section*4+row,y=39+row*39,selected=m.cfg.activeProfile===i;
            this.fill(8,y,222,34,this.card);this.text(selected?'●':'○',16,y+10,13,selected?C.coral:this.dim);this.text(p.name,36,y+10,13,this.fg,'left',185);
            this.hit('select-profile-'+i,'Select profile '+p.name+(selected?', selected':''),8,y,222,34,()=>m.useProfile(i));
            this.button('Edit',238,y,74,34,()=>m.openProfileEditor(i),'quiet','edit-profile-'+i);this.hits[this.hits.length-1].label='Edit profile '+p.name;
          });this.pager(pages);break;
        }
        case 'profile':this.list(m.profile().name,[['Rename profile',()=>{m.scope='profile';m.edit('name');}],...Object.keys(T.profileGroups).map(name=>[name,()=>m.go('fields',{scope:'profile',group:name})]),['Preview profile graph',()=>m.go('graph-preview')],['Load default profile',()=>{m.popup={title:'Load default?',text:'Replace this profile with stock values.',confirm:()=>{m.cfg.profiles[m.editProfile]=T.clone(m.original.profiles[m.editProfile]);}};}]],true);break;
        case 'system':this.list('Machine settings',[...Object.keys(T.systemGroups).map(name=>[name,()=>m.go('fields',{scope:'system',group:name})]),['Water tank calibration',()=>m.go('water')],['Tray guard',()=>m.go('tray')],['Reset all settings',()=>{m.popup={title:'Reset settings?',text:'Restore the backed-up starting values.',confirm:()=>{m.cfg=T.clone(m.original);m.editProfile=m.cfg.activeProfile;m.prefs=T.clone(m.savedPrefs);m.message='Settings reset';}};}]],true);break;
        case 'fields':this.fields(m.group,m.fieldKeys());break;
        case 'boiler':m.scope='profile';m.editProfile=m.cfg.activeProfile;this.list('Temperature',[[`Brew target: ${this.temp} C`,()=>m.edit('setpoint')],[`Steam: ${m.cfg.steamSetPoint} C`,()=>{m.scope='system';m.edit('steamSetPoint');}],[`Offset: ${m.cfg.offsetTemp} C`,()=>{m.scope='system';m.edit('offsetTemp');}],['Heater control settings',()=>m.go('fields',{scope:'system',group:'Boiler'})]],true);break;
        case 'scales':this.top('Scales');this.text(this.weight+' g',160,44,46,this.fg,'center');this.text('NET WEIGHT',160,98,12,this.dim,'center');this.text(`Gross load: ${fmt(m.gross())} g`,160,119,14,this.dim,'center');this.tare(8,153,112,36);this.button(m.scalesView?'SCALES ON':'SCALES OFF',128,153,184,36,()=>{m.scalesView=!m.scalesView;},'blue');this.button('FACTORS / PUMP',8,201,148,33,()=>m.go('fields',{scope:'system',group:'Scales & pump'}));this.button('TRAY GUARD',164,201,148,33,()=>m.go('tray'));break;
        case 'water':this.top('Water tank',true);this.text(m.water()===null?'--':m.water()+'%',14,42,39,m.water()<10?C.red:C.blue);this.text(m.lab.tofValid?m.lab.distance+' mm':'SENSOR INVALID',305,52,18,this.dim,'right');this.waterBar(12,96,296,9);this.text('Full point: '+m.prefs.fullDistance+' mm',12,116,13,this.dim);this.button('SET CURRENT TANK TO 100%',8,143,304,36,()=>{m.popup={title:'Is the tank full?',text:`Use the current ${m.lab.distance} mm as 100%.`,confirm:()=>m.setFull()};},'blue');this.button('FULL POINT',8,201,139,33,()=>{m.scope='system';m.edit('fullDistance');});this.button('LOW LIMIT: 125 mm',155,201,157,33,()=>{m.popup={title:'Low water unchanged',text:'Above 125 mm: 9%, refill warning.'};});break;
        case 'tray':this.top('Tray guard',true);this.text(fmt(m.gross())+' g gross',160,43,31,this.fg,'center');this.text(m.trayReason()||'TRAY READY',160,85,15,m.trayReason()?C.coral:C.blue,'center',300);this.text('Uses gross load, independent of tare',160,111,12,this.dim,'center');this.button('MINIMUM: '+m.prefs.trayMinimum+' g',8,139,304,36,()=>{m.scope='system';m.edit('trayMinimum');});this.button('CAPTURE NO-TRAY ZERO',8,183,304,40,()=>{if(m.isFlushing()){m.popup={title:'Stop flushing first',text:'Calibration requires idle mode.'};return;}if(m.lab.tray||m.gross()>20){m.popup={title:'Remove the tray',text:'Remove tray and any container first.'};return;}if(m.lab.scales!=='ok'){m.popup={title:'Scale unavailable',text:'A fresh hardware reading is required.'};return;}m.prefs.trayCalibrated=true;m.message='No-tray baseline captured';m.popup={title:'Baseline captured',text:'Refit the empty tray, then Save.'};});break;
        case 'cleaning':this.list('Cleaning',[[m.isFlushing()?'Stop group flush':'Group flush',()=>m.flushToggle()],['Backflush cycle',()=>m.go('backflush')],['Descale cycle',()=>m.go('descale')],['Tray guard / calibration',()=>m.go('tray')]]);break;
        case 'backflush':this.top('Backflush');this.text(m.backflush?'CYCLE '+Math.min(11,Math.floor(m.backflushElapsed/5)+1)+' / 11':'BACKFLUSH',160,51,27,C.coral,'center');this.text(m.backflush?(Math.floor(m.backflushElapsed/5)%2?'Rest / drain':'Pump on'):'Fit blind basket; use brew switch',160,96,14,this.dim,'center');this.tray(25,133,275);this.button(m.backflush?'STOP CYCLE':'START CYCLE',8,180,304,47,()=>m.flushToggle(true),m.backflush?'danger':'primary');break;
        case 'descale':this.top('Descale');this.text(m.descale?'DESCALING':'DESCALE',160,47,30,C.coral,'center');this.text(m.descale?`Stage ${Math.floor(m.descaleElapsed/60)+1} / cycle ${Math.floor(m.descaleElapsed/120)+1}`:'Follow the normal cleaning procedure',160,94,13,this.dim,'center');this.text('Brew switch starts / stops cycle',160,122,13,this.dim,'center');this.text('Steam valve and cycle count retained',160,147,12,this.dim,'center');this.button(m.descale?'STOP':'READY FOR BREW SWITCH',8,184,304,43,()=>{if(m.descale)m.switchBrew(false);else m.switchBrew(true);},m.descale?'danger':'primary');break;
        case 'steam':this.top('Steam');this.text(m.cfg.steamSetPoint+' C',160,46,46,C.coral,'center');this.text('Steam target',160,104,13,this.dim,'center');this.text('Physical steam switch remains available',160,135,12,this.dim,'center');this.button(m.steam?'STEAM OFF':'STEAM ON',8,180,304,47,()=>{m.steam=!m.steam;if(m.steam)m.stopFlush();},m.steam?'danger':'primary');break;
        case 'hot-water':this.top('Hot water');this.text(m.hotWater?'DISPENSING':'HOT WATER',160,53,29,C.coral,'center');this.text('Use the physical hot-water switch',160,102,14,this.dim,'center');this.text('Temperature and water level stay visible',160,131,12,this.dim,'center');this.tank(25,157,170);this.button(m.hotWater?'STOP':'START WATER',8,190,304,38,()=>{m.hotWater=!m.hotWater;if(m.hotWater)m.stopFlush();},m.hotWater?'danger':'primary');break;
        case 'manual':{
          this.button('<',4,3,33,26,()=>m.back(),'quiet','back');this.text('Manual flow',44,8,14,this.fg);
          this.button('Auto',252,3,64,26,()=>m.go('shot'),'quiet','manual-auto');this.line(4,32,316,32,C.slate);
          const value=(label,text,x,y,color=this.fg)=>{this.text(label,x,y,10,this.dim,'left',96);this.text(text,x,y+16,25,color,'left',96);};
          value('TEMP °C',Number(m.lab.temperature).toFixed(1),12,43);value('PRESSURE bar',fmt(this.pressure),118,43);value('TIME s',fmt(m.elapsed),224,43,C.coral);
          value('FLOW g/s',fmt(m.pumpFlow),12,103,C.blue);value('WEIGHT g',this.weight,118,103);value('TARGET ml/s',fmt(m.manualFlow),224,103,C.coral);
          this.text(m.brewing?'BREWING · use switch to stop':'Use brew switch to start / stop',12,155,11,this.dim,'left',296);
          const x=18,y=183,w=284,thumb=x+w*m.manualFlow/13;
          this.fill(x,y+12,w,5,C.slate);this.fill(x,y+12,w*m.manualFlow/13,5,C.coral);
          this.ctx.beginPath();this.ctx.arc(thumb,y+14,9,0,Math.PI*2);this.ctx.fillStyle=rgb565(C.coral);this.ctx.fill();
          this.hit('manual-flow-slider','Manual flow target (ml/s)',8,175,304,41,v=>m.setManualFlow(v));this.hits[this.hits.length-1].range={min:0,max:13,step:.1,value:m.manualFlow};
          this.text('0',12,222,10,this.dim);this.text('13 ml/s',308,222,10,this.dim,'right');
          this.text(m.lab.scales==='ok'?'Pump-based flow · manual stop':'Scale unavailable · manual stop',160,222,10,this.dim,'center',220);
          break;
        }
        case 'shot':case 'graph-preview':this.top(m.page==='graph-preview'?'Profile preview':'Shot monitor');this.text(fmt(m.elapsed)+' s',10,41,23,this.fg);this.text(this.weight+' g',309,41,23,C.blue,'right');this.graph(8,77,304,103,m.page==='graph-preview');this.text(`${fmt(this.pressure)} bar / ${fmt(m.pumpFlow)} ml/s / ${this.temp} C`,160,185,11,this.dim,'center');this.tare(8,207,88,27);this.button('MANUAL',104,207,102,27,()=>m.go('manual'));this.button('DOSE',214,207,98,27,()=>m.go('fields',{scope:'profile',group:'Temperature & dose',editProfile:m.cfg.activeProfile}));break;
        case 'led':m.scope='system';this.fields('Lighting',['ledState','ledDisco','ledR','ledG','ledB']);break;
        default:this.list('Menu',[['Home',()=>m.home()]]);
      }
      if(m.isFlushing()&&m.page!=='backflush'){this.fill(39,3,206,26,C.red);this.text('FLUSHING - TAP TO STOP',142,9,12,C.white,'center');this.hit('persistent-flush-stop','Stop flush',39,3,206,26,()=>m.stopFlush());}
    }
    dial(x,y,r,value,max,color){const q=this.ctx;q.lineWidth=8;q.strokeStyle=rgb565(C.slate);q.beginPath();q.arc(x,y,r,Math.PI*.8,Math.PI*2.2);q.stroke();q.strokeStyle=rgb565(color);q.beginPath();q.arc(x,y,r,Math.PI*.8,Math.PI*.8+Math.PI*1.4*Math.min(1,value/max));q.stroke();for(let i=0;i<9;i++){const a=Math.PI*.8+i*Math.PI*1.4/8;this.line(x+Math.cos(a)*(r-12),y+Math.sin(a)*(r-12),x+Math.cos(a)*(r-17),y+Math.sin(a)*(r-17),C.muted);}}
    graph(x,y,w,h,preview=false){
      this.fill(x,y,w,h,this.card);for(let i=1;i<4;i++)this.line(x,y+i*h/4,x+w,y+i*h/4,this.m.concept===3?C.grey:C.slate);
      let a=this.m.curve;if(!a.length||preview){const p=this.m.cfg.profiles[preview?this.m.editProfile:this.m.cfg.activeProfile];a=Array.from({length:40},(_,i)=>({t:i,p:i<6?i/2:i<30?(p.mpProfilingStart||9):Math.max(0,9-(i-30)),f:i<6?.2:1.7,w:Math.max(0,(i-6)*1.7)}));}
      const q=this.ctx;[['p',12,C.coral],['f',6,C.blue],['w',50,(C.light===undefined?this.m.concept===3:C.light)?C.black:C.white]].forEach(([key,max,color])=>{q.strokeStyle=rgb565(color);q.lineWidth=1;q.beginPath();a.forEach((v,i)=>{const px=x+i/Math.max(1,a.length-1)*w,py=y+h-4-Math.min(1,v[key]/max)*(h-8);if(i)q.lineTo(px,py);else q.moveTo(px,py);});q.stroke();});this.text('0',x+3,y+h-14,10,this.dim);this.text(Math.ceil(a[a.length-1].t)+'s',x+w-3,y+h-14,10,this.dim,'right');
    }
    popup(){
      const p=this.m.popup;if(!p)return;this.hits=[];this.fill(0,0,320,240,this.bg);this.fill(8,30,304,172,this.card);this.fill(8,30,304,5,p.kind==='blocked'?C.red:C.blue);this.text(p.title,160,52,23,this.fg,'center',280);
      const words=p.text.split(' ');let lines=[''];for(const word of words){let s=lines[lines.length-1];if((s+' '+word).length>35)lines.push(word);else lines[lines.length-1]=(s?s+' ':'')+word;}
      lines.forEach((s,i)=>this.text(s,160,93+i*20,14,this.dim,'center',280));
      if(p.confirm){this.button('CANCEL',16,153,135,37,()=>{this.m.popup=null;});this.button('CONFIRM',169,153,135,37,()=>{this.m.popup=null;p.confirm();},'blue');}
      else this.button('OK',70,153,180,37,()=>{this.m.popup=null;},'blue');
    }
    keyboard(){
      const e=this.m.editor;if(!e)return;this.hits=[];this.fill(0,0,320,240,this.bg);this.text(e.label,8,7,14,this.dim,'left',300);this.fill(8,29,304,34,this.card);this.text(e.value||' ',16,34,e.string?18:24,this.fg,'left',288);
      if(e.error)this.text(e.error,8,66,11,C.coral,'left',304);
      const rows=e.string?['QWERTYUIOP','ASDFGHJKL','ZXCVBNM']:['123','456','789'];
      rows.forEach((row,r)=>{const w=e.string?29:72,gap=e.string?2:5,left=e.string?6:8;[...row].forEach((k,i)=>this.button(k,left+i*(w+gap),83+r*35,w,31,()=>this.m.key(k),'normal','key-'+k));});
      if(e.string){this.button('SPACE',8,190,119,30,()=>this.m.key('SPACE'));this.button('DEL',134,190,69,30,()=>this.m.key('DEL'));this.button('OK',210,190,102,30,()=>this.m.key('OK'),'blue');this.button('CANCEL',222,49,90,27,()=>this.m.key('CANCEL'),'quiet');}
      else {this.button('0',8,190,72,31,()=>this.m.key('0'));this.button('.',85,190,72,31,()=>this.m.key('.'));this.button('+/-',162,190,72,31,()=>this.m.key('-'));this.button('DEL',241,83,71,31,()=>this.m.key('DEL'));this.button('CLEAR',241,118,71,31,()=>this.m.key('CLEAR'));this.button('BACK',241,153,71,31,()=>this.m.key('CANCEL'));this.button('OK',241,190,71,31,()=>this.m.key('OK'),'blue');}
    }
    syncTargets(){
      const signature=this.hits.map(h=>[h.id,h.label,h.x,h.y,h.w,h.h,h.disabled,!!h.range].join('|')).join(';');
      if(signature===this.signature){(this.rangeControls||[]).forEach(({b,id})=>{const h=this.hits.find(h=>h.id===id);if(h){b.value=h.range.value;b.setAttribute('aria-valuetext',h.range.value.toFixed(1)+' milliliters per second');}});return;}
      this.signature=signature;this.overlay.replaceChildren();this.rangeControls=[];
      this.hits.forEach(h=>{
        const b=document.createElement(h.range?'input':'button');b.type=h.range?'range':'button';b.setAttribute('aria-label',h.label);
        if(h.range){Object.assign(b,h.range);b.className='canvas-slider';b.oninput=()=>{h.action(Number(b.value));this.draw();};this.rangeControls.push({b,id:h.id});b.setAttribute('aria-valuetext',h.range.value.toFixed(1)+' milliliters per second');}
        else {if(typeof h.switchState==='boolean'){b.setAttribute('role','switch');b.setAttribute('aria-checked',String(h.switchState));}b.onclick=()=>{h.action();this.persist();this.draw();};}
        b.title=h.label;b.disabled=h.disabled;b.style.cssText=`left:${h.x}px;top:${h.y}px;width:${h.w}px;height:${h.h}px`;this.overlay.append(b);
      });
    }
    persist(){try{localStorage.setItem((this.palette.storagePrefix||'gaggiuino-trial-')+this.m.concept,JSON.stringify({cfg:this.m.saved,prefs:this.m.savedPrefs}));}catch(e){}}
    draw(){
      const light=C.light===undefined?this.m.concept===3:C.light;this.bg=light?C.white:C.black;this.fg=light?C.black:C.white;this.dim=light?C.slate:C.muted;this.card=light?C.white:C.panel;
      this.hits=[];this.fill(0,0,320,240,this.bg);if(this.m.page==='home')this.home();else this.secondary();if(this.m.editor)this.keyboard();if(this.m.popup)this.popup();
      // The LCD has no alpha channel or 24-bit color: quantize the final raster,
      // including text/arc antialiasing, to RGB565 before presenting it.
      const image=this.ctx.getImageData(0,0,320,240),p=image.data;for(let i=0;i<p.length;i+=4){const r=p[i]>>3,g=p[i+1]>>2,b=p[i+2]>>3;p[i]=r<<3|r>>2;p[i+1]=g<<2|g>>4;p[i+2]=b<<3|b>>2;p[i+3]=255;}this.ctx.putImageData(image,0,0);this.syncTargets();
    }
  }
  return View;
  }
  root.GaggiuinoView={View:createView(),createView,C:BASE_COLORS,rgb565};
})(typeof globalThis!=='undefined'?globalThis:this);
