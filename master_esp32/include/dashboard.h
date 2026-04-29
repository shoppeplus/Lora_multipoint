#ifndef DASHBOARD_H
#define DASHBOARD_H

// ============================================
// Embedded Web Dashboard - Bridge Monitor v3.2
// Fast Continuous Monitoring 
// ============================================

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Bridge Vibration Monitor </title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{
--bg:#0a0e17;--card:#131a2b;--card2:#1a2340;
--accent:#00d4ff;--accent2:#7c3aed;--accent3:#06d6a0;
--warn:#fbbf24;--crit:#ef4444;--ok:#22c55e;
--txt:#e2e8f0;--txt2:#94a3b8;--border:#1e293b;
--axisX:#f87171;--axisY:#4ade80;--axisZ:#38bdf8;
}
body{background:var(--bg);color:var(--txt);font-family:'Segoe UI',system-ui,sans-serif;min-height:100vh}
.header{background:linear-gradient(135deg,#0f172a 0%,#1e1b4b 100%);padding:18px 24px;border-bottom:1px solid var(--border);display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:10px}
.header h1{font-size:1.3em;background:linear-gradient(90deg,var(--accent),var(--accent2));-webkit-background-clip:text;-webkit-text-fill-color:transparent;font-weight:700}
.header-info{display:flex;gap:14px;font-size:0.82em;color:var(--txt2);flex-wrap:wrap}
.header-info span{display:flex;align-items:center;gap:4px}
.dot{width:8px;height:8px;border-radius:50%;display:inline-block}
.dot.on{background:var(--ok);box-shadow:0 0 6px var(--ok)}
.dot.off{background:var(--crit);box-shadow:0 0 6px var(--crit)}
.dot.learn{background:var(--warn);box-shadow:0 0 6px var(--warn);animation:pulse 1.5s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.4}}
@keyframes fadeIn{from{opacity:0.6}to{opacity:1}}
.updated{animation:fadeIn 0.3s ease}
.main{padding:14px;max-width:1440px;margin:0 auto}
.status-bar{display:flex;gap:10px;margin-bottom:14px;flex-wrap:wrap}
.stat{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:12px 16px;flex:1;min-width:100px;text-align:center}
.stat .val{font-size:1.6em;font-weight:700;color:var(--accent)}
.stat .lbl{font-size:0.72em;color:var(--txt2);text-transform:uppercase;letter-spacing:1px;margin-top:2px}
.slaves{display:grid;grid-template-columns:repeat(auto-fit,minmax(420px,1fr));gap:14px;margin-bottom:14px}
.slave-card{background:var(--card);border:1px solid var(--border);border-radius:14px;overflow:hidden;transition:all 0.3s}
.slave-card:hover{border-color:var(--accent);box-shadow:0 0 20px rgba(0,212,255,0.08)}
.slave-card.offline{opacity:0.5}
.slave-card.alert-warn{border-color:var(--warn);box-shadow:0 0 15px rgba(251,191,36,0.15)}
.slave-card.alert-crit{border-color:var(--crit);box-shadow:0 0 20px rgba(239,68,68,0.2)}
.slave-header{padding:14px 18px;display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid var(--border)}
.slave-name{font-weight:600;font-size:1.05em;display:flex;align-items:center;gap:8px}
.badge{padding:3px 10px;border-radius:20px;font-size:0.68em;font-weight:600;text-transform:uppercase;letter-spacing:0.5px}
.badge.ok{background:rgba(34,197,94,0.15);color:var(--ok);border:1px solid rgba(34,197,94,0.3)}
.badge.warn{background:rgba(251,191,36,0.15);color:var(--warn);border:1px solid rgba(251,191,36,0.3)}
.badge.crit{background:rgba(239,68,68,0.15);color:var(--crit);border:1px solid rgba(239,68,68,0.3)}
.badge.off{background:rgba(148,163,184,0.15);color:var(--txt2);border:1px solid rgba(148,163,184,0.3)}
.slave-body{padding:14px 18px}
.axis-section{margin-bottom:10px;background:var(--card2);border-radius:10px;padding:10px 14px}
.axis-section:last-child{margin-bottom:0}
.axis-label{display:flex;align-items:center;gap:6px;margin-bottom:6px;font-size:0.78em;font-weight:600}
.axis-tag{padding:2px 8px;border-radius:4px;font-size:0.75em;font-weight:700;min-width:22px;text-align:center}
.axis-tag.x{background:rgba(248,113,113,0.2);color:var(--axisX)}
.axis-tag.y{background:rgba(74,222,128,0.2);color:var(--axisY)}
.axis-tag.z{background:rgba(56,189,248,0.2);color:var(--axisZ)}
.axis-stats{display:flex;gap:8px;flex-wrap:wrap}
.axis-stat{flex:1;min-width:70px}
.axis-stat .lbl{font-size:0.65em;color:var(--txt2);text-transform:uppercase}
.axis-stat .val{font-size:1em;font-weight:600;font-variant-numeric:tabular-nums;transition:color 0.3s}
.freq-bar-row{display:flex;gap:4px;align-items:flex-end;height:36px;margin-top:4px;margin-bottom:2px}
.freq-bar-row .bar{flex:1;border-radius:3px 3px 0 0;min-height:3px;transition:height 0.5s}
.freq-labels{display:flex;gap:4px}
.freq-labels span{flex:1;text-align:center;font-size:0.62em;color:var(--txt2)}
.rssi-row{display:flex;align-items:center;gap:8px;margin-top:6px;font-size:0.8em}
.rssi-bar{flex:1;height:5px;background:var(--card2);border-radius:3px;overflow:hidden}
.rssi-fill{height:100%;border-radius:3px;transition:width 0.5s}
.alert-reason{margin-top:8px;padding:7px 10px;border-radius:8px;font-size:0.78em;background:rgba(239,68,68,0.1);border:1px solid rgba(239,68,68,0.2);color:var(--crit);display:none}
.alert-reason.show{display:block}
.bottom-bar{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:12px 18px;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:10px}
.bottom-bar .info{font-size:0.78em;color:var(--txt2)}
.refresh-dot{display:inline-block;width:6px;height:6px;border-radius:50%;background:var(--accent);margin-right:4px;animation:pulse 2s infinite}
.btn{padding:7px 14px;border:none;border-radius:8px;cursor:pointer;font-size:0.78em;font-weight:600;transition:all 0.2s}
.btn-reset{background:linear-gradient(135deg,var(--accent2),var(--crit));color:#fff}
.btn-reset:hover{transform:scale(1.05);box-shadow:0 4px 12px rgba(124,58,237,0.3)}
.btn-calib{background:linear-gradient(135deg,var(--accent),var(--accent3));color:#0a0e17}
.btn-calib:hover{transform:scale(1.05);box-shadow:0 4px 12px rgba(0,212,255,0.3)}
.chart-section{background:var(--card);border:1px solid var(--border);border-radius:14px;padding:16px 18px;margin-bottom:14px}
.chart-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;flex-wrap:wrap;gap:8px}
.chart-header h2{font-size:1em;font-weight:600;color:var(--txt)}
.chart-tabs{display:flex;gap:4px}
.chart-tab{padding:4px 12px;border-radius:6px;font-size:0.72em;cursor:pointer;border:1px solid var(--border);background:transparent;color:var(--txt2);transition:all 0.2s}
.chart-tab.active{background:var(--accent);color:#0a0e17;border-color:var(--accent)}
.chart-slave-sel{display:flex;gap:4px;align-items:center;font-size:0.78em;color:var(--txt2)}
.chart-slave-sel select{background:var(--card2);color:var(--txt);border:1px solid var(--border);border-radius:6px;padding:3px 8px;font-size:0.9em}
.chart-canvas{width:100%;height:200px;border-radius:8px;background:var(--card2)}
.chart-info{display:flex;justify-content:space-between;margin-top:6px;font-size:0.7em;color:var(--txt2)}
@media(max-width:600px){
.slaves{grid-template-columns:1fr}
.header h1{font-size:1em}
}
</style>
</head>
<body>
<div class="header">
<h1>🌉 Bridge Vibration Monitor — Real-time</h1>
<div class="header-info">
<span><span class="dot" id="sysDot"></span> <span id="sysStatus">Connecting...</span></span>
<span>⏱ <b id="uptime">--</b></span>
<span>📡 #<b id="cycle">--</b></span>
<span id="timeSpan">🕐 <b id="ntpTime">--</b></span>
<span><span class="refresh-dot"></span><b id="cycleTime">--</b></span>
</div>
</div>
<div class="main">
<div class="status-bar">
<div class="stat"><div class="val" id="onlineCount">-</div><div class="lbl">Online</div></div>
<div class="stat"><div class="val" id="warningCount" style="color:var(--warn)">-</div><div class="lbl">Warnings</div></div>
<div class="stat"><div class="val" id="criticalCount" style="color:var(--crit)">-</div><div class="lbl">Critical</div></div>
<div class="stat"><div class="val" id="pktSuccess" style="color:var(--accent3)">-</div><div class="lbl">Packet %</div></div>
<div class="stat"><div class="val" id="cycleRate" style="color:var(--accent)">-</div><div class="lbl">Cycle/min</div></div>
</div>
<div class="slaves" id="slavesContainer"></div>
<div class="chart-section">
<div class="chart-header">
<h2>📈 History (1 hour)</h2>
<div style="display:flex;gap:12px;align-items:center;flex-wrap:wrap">
<div class="chart-tabs">
<button class="chart-tab active" onclick="setChart('rms')">RMS</button>
<button class="chart-tab" onclick="setChart('peak')">Peak</button>
<button class="chart-tab" onclick="setChart('freq')">Freq</button>
</div>
<div class="chart-slave-sel">Slave: <select id="chartSlave" onchange="loadHistory()"></select></div>
</div>
</div>
<canvas class="chart-canvas" id="histChart"></canvas>
<div class="chart-info"><span id="chartPts">-- points</span><span id="chartRange">--</span></div>
</div>
<div class="bottom-bar">
<div class="info">
<span id="baselineInfo">Baseline: --</span> |
<span><span class="refresh-dot"></span>Auto-refresh: 1s</span> |
<span id="wifiInfo">WiFi: --</span>
</div>
<button class="btn btn-calib" onclick="recalibrate()">🧭 Recalibrate Axes</button>
<button class="btn btn-reset" onclick="resetBaseline()">🔄 Reset Baseline</button>
</div>
</div>
<script>
var lastCycle=0,lastTime=0;
function fmtUp(s){if(s<60)return s+'s';if(s<3600)return Math.floor(s/60)+'m '+s%60+'s';return Math.floor(s/3600)+'h '+Math.floor((s%3600)/60)+'m';}
function rssiClr(r){return r>-60?'var(--ok)':r>-80?'var(--warn)':'var(--crit)';}
function rssiPct(r){return Math.max(0,Math.min(100,(r+120)*100/80));}
function alertBadge(a,on){
if(!on)return '<span class="badge off">OFFLINE</span>';
if(a==2)return '<span class="badge crit">⚠ CRITICAL</span>';
if(a==1)return '<span class="badge warn">⚡ WARNING</span>';
return '<span class="badge ok">✓ NORMAL</span>';
}
function valClr(v,warnTh,critTh){
if(v>=critTh)return 'var(--crit)';
if(v>=warnTh)return 'var(--warn)';
return 'var(--txt)';
}
function cardClass(s){
if(!s.online)return 'slave-card offline';
if(s.alert==2)return 'slave-card alert-crit';
if(s.alert==1)return 'slave-card alert-warn';
return 'slave-card';
}
function buildCard(s){
let on=s.online;
if(!on)return `<div class="${cardClass(s)}">
<div class="slave-header"><span class="slave-name"><span class="dot off"></span>Slave ${s.id}</span>${alertBadge(0,false)}</div>
<div class="slave-body" style="padding:30px 0;text-align:center;color:var(--txt2)">
<div style="font-size:2em;margin-bottom:6px">📡</div><div>Không có tín hiệu</div>
${s.lastSeen>0?`<div style="font-size:0.8em;margin-top:4px">Lần cuối: ${s.lastSeen}s trước</div>`:''}</div></div>`;

let maxFreq=Math.max(s.f1z,s.f2z,s.f3z,0.1);
return `<div class="${cardClass(s)}">
<div class="slave-header">
<span class="slave-name"><span class="dot on"></span>Slave ${s.id} — Điểm đo #${s.id}</span>
${alertBadge(s.alert,true)}
</div>
<div class="slave-body">
<!-- Z AXIS -->
<div class="axis-section">
<div class="axis-label"><span class="axis-tag z">Z</span> Trục đứng (chính)</div>
<div class="freq-bar-row">
<div class="bar" style="height:${(s.f1z/maxFreq*100).toFixed(0)}%;background:linear-gradient(to top,var(--axisZ),rgba(56,189,248,0.3))"></div>
<div class="bar" style="height:${(s.f2z/maxFreq*100).toFixed(0)}%;background:linear-gradient(to top,var(--accent2),rgba(124,58,237,0.3))"></div>
<div class="bar" style="height:${(s.f3z/maxFreq*100).toFixed(0)}%;background:linear-gradient(to top,var(--accent3),rgba(6,214,160,0.3))"></div>
</div>
<div class="freq-labels"><span>f1: ${s.f1z.toFixed(1)} Hz</span><span>f2: ${s.f2z.toFixed(1)} Hz</span><span>f3: ${s.f3z.toFixed(1)} Hz</span></div>
<div class="axis-stats" style="margin-top:6px">
<div class="axis-stat"><div class="lbl">RMS</div><div class="val" style="color:${valClr(s.rmsZ,0.05,0.2)}">${s.rmsZ.toFixed(4)} g</div></div>
<div class="axis-stat"><div class="lbl">Peak</div><div class="val" style="color:${valClr(s.peakZ,1.0,2.0)}">${s.peakZ.toFixed(4)} g</div></div>
<div class="axis-stat"><div class="lbl">Crest Factor</div><div class="val" style="color:${valClr(s.cfZ,5,10)}">${s.cfZ.toFixed(2)}</div></div>
</div>
</div>
<!-- X AXIS -->
<div class="axis-section">
<div class="axis-label"><span class="axis-tag x">X</span> Trục ngang (dọc cầu)</div>
<div class="axis-stats">
<div class="axis-stat"><div class="lbl">Tần số</div><div class="val" style="color:var(--axisX)">${s.freqX.toFixed(1)} Hz</div></div>
<div class="axis-stat"><div class="lbl">RMS</div><div class="val">${s.rmsX.toFixed(4)} g</div></div>
<div class="axis-stat"><div class="lbl">Peak</div><div class="val" style="color:${valClr(s.peakX,1.0,2.0)}">${s.peakX.toFixed(4)} g</div></div>
</div>
</div>
<!-- Y AXIS -->
<div class="axis-section">
<div class="axis-label"><span class="axis-tag y">Y</span> Trục ngang (ngang cầu)</div>
<div class="axis-stats">
<div class="axis-stat"><div class="lbl">Tần số</div><div class="val" style="color:var(--axisY)">${s.freqY.toFixed(1)} Hz</div></div>
<div class="axis-stat"><div class="lbl">RMS</div><div class="val">${s.rmsY.toFixed(4)} g</div></div>
<div class="axis-stat"><div class="lbl">Peak</div><div class="val" style="color:${valClr(s.peakY,1.0,2.0)}">${s.peakY.toFixed(4)} g</div></div>
</div>
</div>
<!-- RSSI -->
<div class="rssi-row">
<span>📶 ${s.rssi} dBm</span>
<div class="rssi-bar"><div class="rssi-fill" style="width:${rssiPct(s.rssi)}%;background:${rssiClr(s.rssi)}"></div></div>
</div>
<div class="alert-reason ${s.alertReasons?'show':''}">${s.alertReasons||''}</div>
</div></div>`;
}
function update(){
fetch('/api/data').then(r=>r.json()).then(d=>{
document.getElementById('cycle').textContent=d.cycle;
document.getElementById('uptime').textContent=fmtUp(d.uptime);
let dot=document.getElementById('sysDot');
let st=document.getElementById('sysStatus');
if(d.baselineReady){dot.className='dot on';st.textContent='MONITORING';}
else{dot.className='dot learn';st.textContent='LEARNING ('+d.baselineProgress+'/10)';}
if(d.ntpTime){document.getElementById('ntpTime').textContent=d.ntpTime;document.getElementById('timeSpan').style.display='';}
else{document.getElementById('timeSpan').style.display='none';}
// Cycle rate
let now=Date.now();
if(lastCycle>0 && d.cycle>lastCycle){
let elapsed=(now-lastTime)/1000;
let rate=((d.cycle-lastCycle)/elapsed*60).toFixed(0);
document.getElementById('cycleRate').textContent=rate;
document.getElementById('cycleTime').textContent=((elapsed/(d.cycle-lastCycle))*1000).toFixed(0)+'ms/cycle';
}
lastCycle=d.cycle;lastTime=now;
let on=0,w=0,c=0,html='';
d.slaves.forEach(s=>{
if(s.online)on++;
if(s.alert==1)w++;
if(s.alert==2)c++;
html+=buildCard(s);
});
document.getElementById('onlineCount').textContent=on+'/'+d.slaves.length;
document.getElementById('warningCount').textContent=w;
document.getElementById('criticalCount').textContent=c;
let container=document.getElementById('slavesContainer');
container.innerHTML=html;
container.classList.remove('updated');
void container.offsetWidth;
container.classList.add('updated');
document.getElementById('baselineInfo').textContent=d.baselineReady?'Baseline: Active ✓':'Baseline: Learning...';
document.getElementById('wifiInfo').textContent='AP: '+d.apIP+(d.staIP?' | STA: '+d.staIP:'');
let tot=d.totalPolls||1,suc=d.successPolls||0;
document.getElementById('pktSuccess').textContent=tot>0?Math.round(suc*100/tot)+'%':'--';
}).catch(()=>{
document.getElementById('sysDot').className='dot off';
document.getElementById('sysStatus').textContent='Disconnected';
});
}
function resetBaseline(){
if(confirm('Reset baseline? Hệ thống sẽ học lại.')){
fetch('/api/reset-baseline',{method:'POST'}).then(()=>update());
}
}
function recalibrate(){
if(confirm('Recalibrate axes?\nCác slave sẽ tự phát hiện lại trục gravity.\nQuá trình mất ~10 giây.')){
fetch('/api/recalibrate',{method:'POST'}).then(r=>r.json()).then(d=>{
if(d.status=='busy'){alert('Đang calibrate, vui lòng chờ...');return;}
alert('Đã gửi lệnh recalibrate!\nĐang xử lý...');
let poll=setInterval(()=>{
fetch('/api/recalibrate-status').then(r=>r.json()).then(s=>{
if(s.done){
clearInterval(poll);
alert('Recalibrate xong!\n\n'+s.acks+'/'+s.total+' slave OK\n'+s.detail);
update();
}
});
},2000);
}).catch(()=>alert('Lỗi kết nối!'));
}
}
// === History Chart ===
var chartMode='rms',chartPoints=[],lastFetchTime=0,chartMax=3600,chartInterval=1;
function setChart(mode){
chartMode=mode;
document.querySelectorAll('.chart-tab').forEach(t=>t.classList.remove('active'));
event.target.classList.add('active');
renderChart();
}
function loadHistory(){
let si=document.getElementById('chartSlave').value||0;
let url='/api/history?slave='+si+'&since='+lastFetchTime;
fetch(url).then(r=>r.json()).then(d=>{
chartMax=d.max;chartInterval=d.interval;
if(d.sent>0){
// Append new points
for(let p of d.points) chartPoints.push(p);
// Update last fetch time
lastFetchTime=chartPoints[chartPoints.length-1][0];
// Trim: keep only last 1 hour
let cutoff=lastFetchTime-3600;
while(chartPoints.length>0&&chartPoints[0][0]<cutoff) chartPoints.shift();
}
let n=chartPoints.length;
document.getElementById('chartPts').textContent=n+'/'+chartMax+' pts ('+chartInterval+'s) [+'+d.sent+']';
if(n>1){
let dur=chartPoints[n-1][0]-chartPoints[0][0];
let m=Math.floor(dur/60),s=dur%60;
document.getElementById('chartRange').textContent='Span: '+m+'m '+s+'s';
} else {document.getElementById('chartRange').textContent='Waiting...';}
renderChart();
}).catch(()=>{});
}
function switchSlave(){
chartPoints=[];lastFetchTime=0;loadHistory();
}
function renderChart(){
if(chartPoints.length<2)return;
let c=document.getElementById('histChart');
let ctx=c.getContext('2d');
let W=c.offsetWidth,H=c.offsetHeight;
c.width=W*2;c.height=H*2;
ctx.scale(2,2);
ctx.clearRect(0,0,W,H);
let pts=chartPoints;
let ci=chartMode=='rms'?1:chartMode=='peak'?2:3;
let vals=pts.map(p=>p[ci]);
let mn=Math.min(...vals),mx=Math.max(...vals);
if(mx==mn){mx=mn+0.001;}
let pad={t:12,b:20,l:50,r:10};
let gW=W-pad.l-pad.r,gH=H-pad.t-pad.b;
// Grid
ctx.strokeStyle='rgba(255,255,255,0.06)';ctx.lineWidth=0.5;
for(let i=0;i<=4;i++){
let y=pad.t+gH*(1-i/4);
ctx.beginPath();ctx.moveTo(pad.l,y);ctx.lineTo(W-pad.r,y);ctx.stroke();
ctx.fillStyle='rgba(255,255,255,0.35)';ctx.font='9px sans-serif';ctx.textAlign='right';
let v=mn+(mx-mn)*i/4;
ctx.fillText(ci==3?v.toFixed(1):v.toFixed(4),pad.l-4,y+3);
}
// Time labels
let t0=pts[0][0],tN=pts[pts.length-1][0],tR=tN-t0||1;
ctx.fillStyle='rgba(255,255,255,0.3)';ctx.textAlign='center';
for(let i=0;i<=4;i++){
let t=t0+tR*i/4;
let x=pad.l+gW*i/4;
let m=Math.floor(t/60)%60,s=t%60;
ctx.fillText(m+':'+(s<10?'0':'')+s,x,H-4);
}
// Line
ctx.beginPath();
let colors={rms:'56,189,248',peak:'239,68,68',freq:'6,214,160'};
let clr=colors[chartMode]||'56,189,248';
for(let i=0;i<pts.length;i++){
let x=pad.l+gW*((pts[i][0]-t0)/tR);
let y=pad.t+gH*(1-(vals[i]-mn)/(mx-mn));
if(i==0)ctx.moveTo(x,y);else ctx.lineTo(x,y);
}
ctx.strokeStyle='rgb('+clr+')';ctx.lineWidth=1.5;ctx.stroke();
// Fill
ctx.lineTo(pad.l+gW,pad.t+gH);ctx.lineTo(pad.l,pad.t+gH);ctx.closePath();
ctx.fillStyle='rgba('+clr+',0.08)';ctx.fill();
// Alert dots
for(let i=0;i<pts.length;i++){
if(pts[i][4]>=2){
let x=pad.l+gW*((pts[i][0]-t0)/tR);
let y=pad.t+gH*(1-(vals[i]-mn)/(mx-mn));
ctx.beginPath();ctx.arc(x,y,3,0,Math.PI*2);
ctx.fillStyle='#ef4444';ctx.fill();
} else if(pts[i][4]==1){
let x=pad.l+gW*((pts[i][0]-t0)/tR);
let y=pad.t+gH*(1-(vals[i]-mn)/(mx-mn));
ctx.beginPath();ctx.arc(x,y,2,0,Math.PI*2);
ctx.fillStyle='#fbbf24';ctx.fill();
}
}
// Unit + latest value
ctx.fillStyle='rgba(255,255,255,0.5)';ctx.font='bold 10px sans-serif';ctx.textAlign='left';
let unit=chartMode=='freq'?'Hz':'g';
let latest=vals[vals.length-1];
ctx.fillText(unit+' | now: '+(ci==3?latest.toFixed(1):latest.toFixed(4)),pad.l+4,pad.t+10);
}
function initChartSlave(){
let sel=document.getElementById('chartSlave');
for(let i=0;i<2;i++){let o=document.createElement('option');o.value=i;o.textContent='S'+(i+1);sel.appendChild(o);}
sel.onchange=function(){switchSlave();};
}
initChartSlave();
update();
setInterval(update,1000);
loadHistory();
setInterval(loadHistory,2000);
</script>
</body>
</html>
)rawliteral";

#endif // DASHBOARD_H
