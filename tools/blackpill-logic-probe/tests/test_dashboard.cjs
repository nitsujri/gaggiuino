// Control-state tests with simulated I/O. Hardware capture is checked separately.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '..', 'dashboard.html'), 'utf8')
  .match(/<script>([\s\S]*?)<\/script>/)[1];
const elements = new Map(), requests = [], timers = new Map();
let nextTimer = 0;
function element(id) {
  if (!elements.has(id)) elements.set(id, {
    value: ({duration:'250', timebase:'fit', trigger:'free', zoom:'0', position:'0',
      mode:'digital',preset:'0',samples:'32768',vertical:'full',threshold:'1.65'})[id] || '',
    checked: id === 'channel0', children: [], clientWidth:1100,
    append(item) { this.children.push(item); }, replaceChildren() { this.children=[]; },
    addEventListener() {}, setPointerCapture() {}, setAttribute() {},
    getContext() { return new Proxy({}, {get:()=>()=>{}}); },
  });
  return elements.get(id);
}
const context = vm.createContext({
  document:{getElementById:element, createElement:()=>element(Symbol())},
  window:{}, devicePixelRatio:1, performance, Date, console, Blob, URL,
  setTimeout(fn) { const id=++nextTimer; timers.set(id,fn); return id; },
  clearTimeout(id) { timers.delete(id); },
  fetch(_url, options) {
    if (!options) return Promise.resolve({json:async()=>({identity:'TEST',capture:null})});
    return new Promise(resolve=>requests.push(resolve));
  },
});
const run=code=>vm.runInContext(code,context);
const settle=()=>new Promise(resolve=>setImmediate(resolve));
function fixture(edge=false) {
  const channels=['CLK','D1','D2'].map((name,i)=>({name,initial:0,final:0,
    rises:edge&&i===0?1:0,falls:edge&&i===0?1:0,high_percent:0}));
  return {identity:'TEST',capture:{hz:1000000,duration_ticks:250000,
    max_poll_gap_ticks:1,truncated:false,channels,
    events:edge?[[0,0],[100,1],[102,0],[250000,0]]:[[0,0],[250000,0]]}};
}
async function finish(result) {
  assert.ok(requests.length,'capture request was issued');
  requests.shift()({ok:true,json:async()=>result}); await settle();
}
function nextFrame() {
  assert.ok(timers.size,'live capture queued next frame');
  const [id,fn]=timers.entries().next().value;timers.delete(id);fn();
}
(async()=>{
  run(source);await settle();
  run('setRunning(true)');run('setRunning(false)');await finish(fixture(true));
  assert.equal(run('data'),null,'Pause must hold the screen during an in-flight request');
  assert.equal(timers.size,0,'Pause must cancel continuous scheduling');
  assert.equal(run('history.length'),1,'in-flight result remains available in history');

  run('setRunning(true)');await finish(fixture());
  element('zoom').value='8';element('zoom').oninput();
  const zoomedSpan=run('span');nextFrame();await finish(fixture());
  assert.equal(run('span'),zoomedSpan,'live updates must preserve manual zoom');
  run('setRunning(false)');

  element('timebase').value='2';element('trigger').value='rising';
  element('holdedge').checked=true;run('setRunning(true)');await finish(fixture(true));
  assert.equal(run('running'),false,'freeze on edge must stop acquisition requests');
  assert.equal(run('held'),true);assert.equal(timers.size,0);
  assert.ok(Math.abs(run('span')-20)<.01,'2 µs/div must span 20 µs');
  assert.ok(Math.abs(run('left')-96)<.01,'rising edge aligns at 20% of the view');
  assert.equal(element('stats').children[1].children[1].textContent,'Hidden');

  for(let i=0;i<40;i++)run(`storeFrame(${JSON.stringify(fixture())},10)`);
  assert.equal(run('history.length'),32,'history memory must remain bounded');
  const analogFrame={identity:'TEST analog=1',capture:{kind:'analog',channel:'PA0',bits:6,count:256,
    sample_hz:4000000,vdda_mv:3300,vref_calibrated:true,quantization_mv:3300/63,
    min_v:0,max_v:3.3,mean_v:1.65,samples:Array.from({length:256},(_,i)=>i<100?0:63)}};
  element('mode').value='analog';element('timebase').value='fit';
  run(`display(storeFrame(${JSON.stringify(analogFrame)},20))`);
  assert.match(element('summary').textContent,/4.000 MS\/s/);
  assert.match(element('stats').children[0].children[2].textContent,/3.300 V/);
  assert.equal(run('clockEdge(data,"rising")'),25,'analog threshold crossing uses sample time');
  element('vertical').value='auto';run('draw()');
  // Exercise peak-preserving bins, rather than averaging narrow spikes away.
  analogFrame.capture.samples=Array(32768).fill(0);analogFrame.capture.samples[100]=63;
  analogFrame.capture.count=32768;
  run(`display(storeFrame(${JSON.stringify(analogFrame)},20))`);
  console.log('PASS: pause race, live zoom, edge hold/alignment, history bounds, analog voltage rendering/threshold/envelope');
})().catch(error=>{console.error(error);process.exitCode=1;});
