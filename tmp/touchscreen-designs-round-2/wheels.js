(function(root){
  'use strict';
  const V=root.GaggiuinoView;
  function color(s){const n=parseInt(s.replace('#',''),16);return ((n>>19&31)<<11)|((n>>10&63)<<5)|(n>>3&31);}
  const palettes=[
    {name:'Lagoon',bg:'#081f24',fg:'#edf7ed',panel:'#14373b',line:'#31575a',muted:'#a6c3bc',accent:'#4ddcca',water:'#87c9e3'},
    {name:'Brass',bg:'#171a16',fg:'#f5ecd7',panel:'#282e25',line:'#555a43',muted:'#bbc0a6',accent:'#e7c05c',water:'#89b9a4'},
    {name:'Linen',light:true,bg:'#f4f0df',fg:'#263e34',panel:'#dedfca',line:'#789383',muted:'#b9c9b7',accent:'#397a58',water:'#4a8e9f'},
    {name:'Cobalt',bg:'#08152c',fg:'#ecf1ff',panel:'#122443',line:'#2e4a70',muted:'#a3b9d5',accent:'#579dff',water:'#6dd8e3'},
    {name:'Mineral',light:true,bg:'#e7e9df',fg:'#263f40',panel:'#d0d9d1',line:'#6a8784',muted:'#b4c7be',accent:'#377e78',water:'#4d8fae'},
    {name:'Iris',bg:'#19162d',fg:'#efebff',panel:'#2d2848',line:'#514a70',muted:'#bdb4d6',accent:'#b5a3f3',water:'#8cbec7'},
    {name:'Sea glass',light:true,bg:'#cee4d7',fg:'#21463f',panel:'#bad4c7',line:'#648e7b',muted:'#9bbcad',accent:'#286b59',water:'#3a8595'},
    {name:'Glacier',bg:'#0c1e30',fg:'#e9f5ff',panel:'#183047',line:'#34546a',muted:'#a2bdca',accent:'#9ed8f0',water:'#5eaccc'},
    {name:'Matcha',bg:'#151d12',fg:'#f0f1dc',panel:'#283423',line:'#4a5d3a',muted:'#b9c3a0',accent:'#b6d36e',water:'#88b8a1'},
    {name:'Walnut',bg:'#241e17',fg:'#f2e5cd',panel:'#3b3227',line:'#675941',muted:'#bfb091',accent:'#d7bc83',water:'#9cb6ac'}
  ];
  function theme(p){return {light:!!p.light,storagePrefix:'gaggiuino-wheel-trial-',black:color(p.light?p.fg:p.bg),white:color(p.light?p.bg:p.fg),panel:color(p.panel),slate:color(p.line),muted:color(p.muted),grey:color(p.panel),coral:color(p.accent),blue:color(p.water),red:color('#d6ac54'),yellow:color('#d6ac54'),green:color(p.accent)};}
  const classes=palettes.map((p,i)=>{
    const Base=V.createView(theme(p));
    return class WheelView extends Base {
      get currentTemp(){return Number.isFinite(this.m.lab.temperature)?this.m.lab.temperature:74.6;}
      get warmStatus(){return this.m.isFlushing()?'FLUSHING':this.m.brewing?'BREWING':this.currentTemp<this.temp-2?'HEATING':this.m.steam?'STEAM':'READY';}
      wheel(cx,cy,r=64,segmented=false){
        const q=this.ctx,C=this.palette,ratio=Math.max(0,Math.min(1,this.currentTemp/this.temp));
        q.fillStyle=V.rgb565(C.panel);q.beginPath();q.arc(cx,cy,r-13,0,Math.PI*2);q.fill();
        q.lineWidth=8;
        if(segmented){
          for(let n=0;n<28;n++){q.strokeStyle=V.rgb565(n/28<ratio?C.coral:C.slate);q.beginPath();q.arc(cx,cy,r-4,-Math.PI/2+n*Math.PI*2/28+.025,-Math.PI/2+(n+1)*Math.PI*2/28-.025);q.stroke();}
        }else{
          q.strokeStyle=V.rgb565(C.slate);q.beginPath();q.arc(cx,cy,r-4,0,Math.PI*2);q.stroke();q.strokeStyle=V.rgb565(C.coral);q.beginPath();q.arc(cx,cy,r-4,-Math.PI/2,-Math.PI/2+ratio*Math.PI*2);q.stroke();
        }
        this.text(this.warmStatus,cx,cy-40,10,this.dim,'center',r*1.5);
        this.text(this.currentTemp.toFixed(1),cx,cy-23,r>=64?35:33,this.fg,'center',r*1.65);
        this.text('°C',cx,cy+16,13,this.fg,'center');this.text('SET '+this.temp+'°',cx,cy+36,11,this.dim,'center');
        this.hit('temperature-wheel','Temperature settings',cx-r,cy-r,r*2,r*2,()=>this.m.go('boiler'));
      }
      waterColumn(x,y,w,h,{outline=false,ticks=false}={}){
        const C=this.palette,pct=this.m.water();this.fill(x,y,w,h,outline?C.slate:C.panel);
        const pad=outline?2:0;this.fill(x+pad,y+pad,w-2*pad,h-2*pad,this.bg);
        if(pct!==null){const height=Math.round((h-2*pad)*pct/100);this.fill(x+pad,y+h-pad-height,w-2*pad,height,pct<10?C.red:C.blue);}
        if(ticks)for(let n=0;n<5;n++)this.line(x+w+3,y+n*h/4,x+w+7,y+n*h/4,C.muted);
        this.hit('water-column','Water tank calibration',x-5,y-8,w+10,h+16,()=>this.m.go('water'));
      }
      readout(label,value,x,y,w=70){this.text(label,x,y,10,this.dim,'left',w);this.text(value,x,y+15,21,this.fg,'left',w);}
      waterLabel(x,y,w=80,center=false){const p=this.m.water();this.text(p===null?'WATER --':`WATER ${p}%`,x,y,11,p!==null&&p<10?this.palette.red:this.dim,center?'center':'left',w);}
      footer(){
        const C=this.palette,q=this.ctx,on=this.m.isFlushing();this.line(8,199,312,199,C.slate);
        this.text('Flush',18,212,14,on?this.fg:this.dim);q.beginPath();q.arc(65,218,5,0,Math.PI*2);
        if(on){q.fillStyle=V.rgb565(C.coral);q.fill();}else{q.strokeStyle=V.rgb565(this.dim);q.lineWidth=1;q.stroke();}
        this.hit('home-flush','Flush '+(on?'on':'off'),8,201,85,38,()=>this.m.flushToggle());this.hits[this.hits.length-1].switchState=on;
        this.button('Tare',160,202,72,32,()=>this.m.tare(),'quiet','tare');this.button('Menu',240,202,72,32,()=>this.m.go('menu'),'quiet','menu');
      }
      home(){
        const m=this.m,C=this.palette;this.text('PROFILE',10,6,9,this.dim);this.text(m.cfg.profiles[m.cfg.activeProfile].name,10,19,14,this.fg,'left',300);
        switch(i){
          case 0:
            this.wheel(111,108,65);this.readout('WEIGHT',this.weight+' g',199,56,68);this.readout('PRESSURE',this.pressure.toFixed(1)+' bar',199,112,71);this.waterLabel(244,32,72);this.waterColumn(286,57,16,105);this.text((m.water()??'--')+'%',294,170,13,C.blue,'center');break;
          case 1:
            this.wheel(161,109,65);this.readout('WEIGHT',this.weight+' g',9,65,75);this.readout('PRESSURE',this.pressure.toFixed(1),9,120,65);this.text('bar',9,157,11,this.dim);this.waterLabel(252,34,65);this.waterColumn(293,58,10,108);this.text((m.water()??'--')+'%',297,176,12,C.blue,'center');break;
          case 2:
            this.wheel(102,111,65);this.readout('WEIGHT',this.weight+' g',192,59,80);this.readout('PRESSURE',this.pressure.toFixed(1)+' bar',192,115,80);this.text('H2O',294,41,10,this.dim,'center');this.waterColumn(286,62,16,105,{outline:true});this.text((m.water()??'--')+'%',294,177,12,C.blue,'center');break;
          case 3:
            this.wheel(156,109,65);this.text('H2O',23,41,10,this.dim,'center');this.waterColumn(17,61,12,100);this.text((m.water()??'--')+'%',23,172,12,C.blue,'center');this.readout('WEIGHT',this.weight,250,65,64);this.text('g',250,102,11,this.dim);this.readout('PRESSURE',this.pressure.toFixed(1),250,125,64);this.text('bar',250,162,11,this.dim);break;
          case 4:
            this.wheel(110,111,65);this.readout('WEIGHT',this.weight+' g',198,55,76);this.readout('PRESSURE',this.pressure.toFixed(1)+' bar',198,110,78);this.text('WATER',277,32,11,this.dim,'center');this.waterColumn(286,58,20,110,{outline:true});this.text((m.water()??'--')+'%',295,177,12,C.blue,'center');break;
          case 5:
            this.wheel(161,109,65);this.readout('WEIGHT',this.weight+' g',9,65,75);this.readout('PRESSURE',this.pressure.toFixed(1)+' bar',9,123,75);this.text('WATER',279,41,10,this.dim,'center');this.waterColumn(271,62,16,105,{outline:true});this.text((m.water()??'--')+'%',279,177,12,C.blue,'center');break;
          case 6:
            this.wheel(207,107,64);this.text('WATER',9,32,11,this.dim);this.waterColumn(17,56,17,112,{outline:true});this.text((m.water()??'--')+'%',25,177,12,C.blue,'center');this.readout('WEIGHT',this.weight+' g',57,65,76);this.readout('PRESSURE',this.pressure.toFixed(1),57,119,72);this.text('bar',57,155,11,this.dim);break;
          case 7:
            this.wheel(112,104,64);this.text('TANK',268,36,11,this.dim,'center');this.waterColumn(252,56,32,111,{outline:true});this.text((m.water()??'--')+'%',268,175,15,C.blue,'center');this.text(this.weight+' g',80,179,16,this.fg,'center');this.hit('weight','Scales',25,172,105,25,()=>m.go('scales'));this.text(this.pressure.toFixed(1)+' bar',176,179,15,this.dim,'center');break;
          case 8:
            this.wheel(155,108,65,true);this.text('H2O',19,35,10,this.dim,'center');this.waterColumn(12,56,12,114,{ticks:true});this.text((m.water()??'--')+'%',21,180,12,C.blue,'center');this.readout('WEIGHT',this.weight,249,65,66);this.text('g',249,102,11,this.dim);this.readout('PRESSURE',this.pressure.toFixed(1),249,121,66);this.text('bar',249,158,11,this.dim);break;
          case 9:
            this.wheel(110,105,64);this.readout('WEIGHT',this.weight+' g',197,55,78);this.readout('PRESSURE',this.pressure.toFixed(1)+' bar',197,111,78);this.waterLabel(246,32,72);this.waterColumn(287,57,14,109);this.text((m.water()??'--')+'%',294,175,12,C.blue,'center');break;
        }
        this.footer();
        // Metric labels remain secondary; the wheel and water are always the anchors.
        if(!this.hits.some(h=>h.id==='weight')){
          const r=i===6?{x:52,y:57,w:82,h:51}:i===1?{x:6,y:57,w:80,h:51}:i===2?{x:187,y:51,w:87,h:51}:i===3||i===8?{x:245,y:57,w:70,h:51}:i===5?{x:6,y:57,w:80,h:51}:{x:193,y:49,w:83,h:51};
          this.hit('weight','Scales',r.x,r.y,r.w,r.h,()=>m.go('scales'));
        }
        if(m.water()!==null&&m.water()<10){this.fill(0,0,320,25,C.red);this.text('LOW WATER - FILL TANK',160,5,13,C.black,'center');}
      }
    };
  });
  V.View=class {constructor(container,model){return new classes[model.concept](container,model);}};
  V.wheelPalettes=palettes;V.wheelClasses=classes;V.color565=color;
})(typeof globalThis!=='undefined'?globalThis:this);
