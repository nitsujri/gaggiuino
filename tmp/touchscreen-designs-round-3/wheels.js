(function(root){
  'use strict';
  const V=root.GaggiuinoView;
  function color(s){const n=parseInt(s.replace('#',''),16);return ((n>>19&31)<<11)|((n>>10&63)<<5)|(n>>3&31);}
  const palettes=[
    {id:'tidal',name:'Tidal',bg:'#071f25',fg:'#eaf6ec',panel:'#13383d',line:'#315b60',muted:'#a6c7c1',accent:'#5ce0c5',water:'#8dd4e3',radius:65,thickness:8,nav:'words',description:'Lagoon proportions · 130 px wheel / 8 px ring · simple Back, Previous, Next labels.'},
    {id:'arctic',name:'Arctic',bg:'#0b2030',fg:'#edf8ff',panel:'#18394c',line:'#386174',muted:'#adc9d7',accent:'#79ccf5',water:'#a6e3e3',radius:70,thickness:10,nav:'chevrons',description:'Glacial blue · 140 px wheel / 10 px ring · clean drawn chevrons.'},
    {id:'celadon',name:'Celadon',bg:'#15251d',fg:'#f3f5e5',panel:'#293e30',line:'#4c6b53',muted:'#bdcdb7',accent:'#b9df9e',water:'#80c7c2',radius:74,thickness:12,nav:'pills',description:'Soft green · 148 px wheel / 12 px ring · rounded navigation pills.'},
    {id:'petrol',name:'Petrol',bg:'#092b32',fg:'#eff6dd',panel:'#19444a',line:'#3f6669',muted:'#b0c7b9',accent:'#d5de77',water:'#82cdd5',radius:78,thickness:14,nav:'outline',description:'Petrol and citron · 156 px wheel / 14 px ring · lightly outlined navigation.'},
    {id:'abyss',name:'Abyss',bg:'#081c2b',fg:'#e8f8f5',panel:'#11384b',line:'#2d596d',muted:'#a2c6cc',accent:'#43d5e0',water:'#8faef3',radius:80,thickness:18,nav:'circles',description:'Deep blue and cyan · 160 px wheel / 18 px ring · circular arrow controls.'},
    {id:'orchid',name:'Orchid',bg:'#211b30',fg:'#f1ecfa',panel:'#37304a',line:'#5b5171',muted:'#c5bcd7',accent:'#c6b2ef',water:'#88cec8',radius:76,thickness:20,nav:'underline',description:'Smoky violet · 152 px wheel / 20 px ring · understated text and underlines.'},
    {id:'porcelain',name:'Porcelain',light:true,bg:'#eaf1e8',fg:'#153e3d',panel:'#d8e5dc',line:'#668c83',muted:'#bdcfc5',accent:'#287d70',water:'#418da4',radius:80,thickness:12,nav:'rail',description:'Pale mineral and teal · 160 px wheel / 12 px ring · flat navigation with side markers.'},
    {id:'indigo',name:'Indigo',bg:'#101c35',fg:'#edf3ff',panel:'#213451',line:'#405c7b',muted:'#aebfd7',accent:'#9caef9',water:'#77d6d9',radius:72,thickness:16,nav:'text-chevron',description:'Blue-violet on navy · 144 px wheel / 16 px ring · chevron plus text.'},
    {id:'reed',name:'Reed',bg:'#20291e',fg:'#f5f1d9',panel:'#35412e',line:'#626f4a',muted:'#c5c9a8',accent:'#ddda8d',water:'#8dc8b8',radius:78,thickness:22,nav:'soft',description:'Moss and pale gold · 156 px wheel / 22 px ring · soft filled navigation tabs.'},
    {id:'pearl',name:'Pearl',bg:'#1b2930',fg:'#f0f0e5',panel:'#304149',line:'#536970',muted:'#bacacb',accent:'#d7e7df',water:'#8fc5de',radius:80,thickness:24,nav:'compact',description:'Pearl on slate · 160 px wheel / 24 px ring · compact arrows with short labels.'}
  ];
  function theme(p){return {light:!!p.light,storagePrefix:'gaggiuino-lagoon-trial-',black:color(p.light?p.fg:p.bg),white:color(p.light?p.bg:p.fg),panel:color(p.panel),slate:color(p.line),muted:color(p.muted),grey:color(p.panel),coral:color(p.accent),blue:color(p.water),red:color('#d6ac54'),yellow:color('#d6ac54'),green:color(p.accent)};}
  const classes=palettes.map(p=>{
    const Base=V.createView(theme(p));
    return class LagoonView extends Base {
      get titleStart(){return ['chevrons','circles','rail','compact'].includes(p.nav)?52:80;}
      get currentTemp(){return Number.isFinite(this.m.lab.temperature)?this.m.lab.temperature:74.6;}
      get warmStatus(){return this.m.isFlushing()?'FLUSHING':this.m.brewing?'BREWING':this.m.steam?'STEAM':this.currentTemp<this.temp-2?'HEATING':'READY';}
      rounded(x,y,w,h,r,c){
        this.fill(x+r,y,w-r*2,h,c);this.fill(x,y+r,w,h-r*2,c);const q=this.ctx;q.fillStyle=V.rgb565(c);
        [[x+r,y+r],[x+w-r,y+r],[x+r,y+h-r],[x+w-r,y+h-r]].forEach(([cx,cy])=>{q.beginPath();q.arc(cx,cy,r,0,Math.PI*2);q.fill();});
      }
      chevron(cx,cy,direction,c,size=5){const d=direction==='right'?1:-1;this.line(cx-d*size/2,cy-size,cx+d*size/2,cy,c,2);this.line(cx+d*size/2,cy,cx-d*size/2,cy+size,c,2);}
      navigation(id,x,y,w,h,direction,action,disabled=false){
        const C=this.palette,c=disabled?C.slate:this.fg,small=id==='back',word=small?'Back':direction==='left'?'Previous':'Next',short=small?'Back':direction==='left'?'Prev':'Next';
        const cx=x+w/2,cy=y+h/2;
        if(p.nav==='pills'||p.nav==='soft')this.rounded(x,y,w,h,p.nav==='pills'?h/2:6,p.light?C.muted:C.panel);
        if(p.nav==='outline'){
          this.rounded(x,y,w,h,6,C.slate);this.rounded(x+1,y+1,w-2,h-2,5,this.bg);
        }
        if(p.nav==='rail'){this.fill(x,y,w,h,C.panel);this.fill(direction==='left'?x:x+w-2,y,2,h,disabled?C.slate:C.coral);}
        if(p.nav==='circles'){
          const r=Math.min(w,h)/2-1,q=this.ctx;q.beginPath();q.arc(cx,cy,r,0,Math.PI*2);q.strokeStyle=V.rgb565(disabled?C.slate:C.coral);q.lineWidth=1;q.stroke();this.chevron(cx,cy,direction,c,5);
        }else if(p.nav==='chevrons'||p.nav==='rail'&&small){this.chevron(cx,cy,direction,c,6);}
        else if(p.nav==='text-chevron'||p.nav==='compact'){
          this.chevron(direction==='left'?x+8:x+w-9,cy,direction,c,4);if(p.nav!=='compact'||!small)this.text(short,cx+(direction==='left'?5:-5),cy-6,12,c,'center',w-22);
        }else{
          this.text(p.nav==='pills'||p.nav==='outline'?short:word,cx,cy-6,12,c,'center',w-8);
          if(p.nav==='underline')this.line(x+5,y+h-1,x+w-5,y+h-1,disabled?C.slate:C.coral);
        }
        this.hit(id,(small?'Back':direction==='left'?'Previous page':'Next page'),x,y,w,h,action,disabled);
      }
      backControl(){this.navigation('back',8,3,this.titleStart-16,26,'left',()=>this.m.back());}
      pager(total){
        const m=this.m,pages=Math.max(1,total),circle=p.nav==='circles';
        this.navigation('previous',circle?32:8,201,circle?34:91,33,'left',()=>{m.section=Math.max(0,m.section-1);},m.section===0);
        this.text(`${m.section+1} / ${pages}`,160,212,12,this.dim,'center');
        this.navigation('next',circle?254:221,201,circle?34:91,33,'right',()=>{m.section=Math.min(pages-1,m.section+1);},m.section>=pages-1);
      }
      wheel(){
        const C=this.palette,q=this.ctx,r=p.radius,t=p.thickness,cx=r>=74?102:110,cy=r>=74?116:111,ratio=Math.max(0,Math.min(1,this.currentTemp/this.temp));
        q.beginPath();q.arc(cx,cy,r-t-5,0,Math.PI*2);q.fillStyle=V.rgb565(C.panel);q.fill();
        q.lineWidth=t;q.strokeStyle=V.rgb565(C.slate);q.beginPath();q.arc(cx,cy,r-t/2,0,Math.PI*2);q.stroke();
        q.strokeStyle=V.rgb565(C.coral);q.beginPath();q.arc(cx,cy,r-t/2,-Math.PI/2,-Math.PI/2+ratio*Math.PI*2);q.stroke();
        const font=r>=74?39:35;
        this.text(this.warmStatus,cx,cy-40,9,this.dim,'center',95);this.text(this.currentTemp.toFixed(1),cx,cy-23,font,this.fg,'center',r*1.5);
        this.text('°C',cx,cy+21,12,this.fg,'center');this.text('SET '+this.temp+'°',cx,cy+37,10,this.dim,'center');
        this.hit('temperature-wheel','Temperature settings',cx-r,cy-r,r*2,r*2,()=>this.m.go('boiler'));
      }
      footer(){
        const C=this.palette,q=this.ctx,on=this.m.isFlushing();this.line(8,199,312,199,C.slate);
        this.text('Flush',18,212,14,on?this.fg:this.dim);q.beginPath();q.arc(65,218,5,0,Math.PI*2);
        if(on){q.fillStyle=V.rgb565(C.coral);q.fill();}else{q.strokeStyle=V.rgb565(this.dim);q.lineWidth=1;q.stroke();}
        this.hit('home-flush','Flush '+(on?'on':'off'),8,201,85,38,()=>this.m.flushToggle());this.hits[this.hits.length-1].switchState=on;
        this.button('Tare',160,202,72,32,()=>this.m.tare(),'quiet','tare');this.button('Menu',240,202,72,32,()=>this.m.go('menu'),'quiet','menu');
      }
      home(){
        const m=this.m,C=this.palette,pct=m.water();this.text('PROFILE',10,6,9,this.dim);this.text(m.cfg.profiles[m.cfg.activeProfile].name,10,19,14,this.fg,'left',300);
        this.wheel();this.text('WEIGHT',196,59,10,this.dim);this.text(this.weight+' g',196,74,21,this.fg,'left',80);
        this.text('PRESSURE',196,119,10,this.dim);this.text(this.pressure.toFixed(1)+' bar',196,134,21,this.fg,'left',83);
        this.hit('weight','Scales',191,51,87,51,()=>m.go('scales'));
        this.text('WATER',308,39,9,this.dim,'right');this.fill(288,58,16,110,C.slate);this.fill(290,60,12,106,C.panel);
        if(pct!==null){const height=Math.round(106*pct/100);this.fill(290,166-height,12,height,pct<10?C.red:C.blue);}
        this.text((pct??'--')+'%',308,177,12,pct!==null&&pct<10?C.red:C.blue,'right');this.hit('water-column','Water tank calibration',283,49,26,143,()=>m.go('water'));
        this.footer();if(pct!==null&&pct<10){this.fill(0,0,320,25,C.red);this.text('LOW WATER - FILL TANK',160,5,13,C.black,'center');}
      }
    };
  });
  V.View=class {constructor(container,model){return new classes[model.concept](container,model);}};
  V.wheelPalettes=palettes;V.wheelClasses=classes;V.color565=color;
})(typeof globalThis!=='undefined'?globalThis:this);
