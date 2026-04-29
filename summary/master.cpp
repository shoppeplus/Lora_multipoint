/* 
 * MONOLITHIC MERGE OF d:\TIm\master_esp32
 */

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <LoRa.h>
#include <Preferences.h>
#include <SPI.h>
#include <WiFi.h>
#include <math.h>
#include <time.h>


// ============================================
// FILE: config.h
// ============================================

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Master ESP32 — Bridge Vibration Monitor
// ============================================

// --- LoRa RA-02 (SX1278) — SPI ---
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  2
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23

// --- LoRa RF (must match Slave) ---
#define LORA_FREQ          433E6
#define LORA_SF            7
#define LORA_BW            125E3
#define LORA_CR            5
#define LORA_TX_POWER      17

// --- Polling Protocol ---
#define NUM_SLAVES         2
#define POLL_TIMEOUT_MS    600     // Wait for slave response
#define POLL_INTERVAL_MS   200     // Pause between full cycles
#define GUARD_TIME_MS      50      // Guard between polling slaves

// --- Retry ---
#define MAX_RETRIES        1
#define RETRY_DELAY_MS     100

// --- Multi-Frequency ---
#define NUM_TOP_FREQS      3

// --- Baseline & Anomaly Detection ---
#define BASELINE_CYCLES    10      // Learning period (cycles)
#define ANOMALY_SIGMA      3.0f   // Z-score threshold

// --- Alert Levels ---
#define ALERT_NORMAL    0
#define ALERT_WARNING   1
#define ALERT_CRITICAL  2

// --- WiFi ---
#define WIFI_AP_SSID       "BridgeMonitor"
#define WIFI_AP_PASS       "bridge2024"
#define WIFI_STA_SSID      "CUONG THU"
#define WIFI_STA_PASS      "00000000"
#define WIFI_AP_CHANNEL    6
#define WIFI_AP_MAX_CONN   4

// --- NTP ---
#define NTP_SERVER         "pool.ntp.org"
#define NTP_GMT_OFFSET     25200   // UTC+7
#define NTP_DAYLIGHT       0

// --- History Ring Buffer (RAM) ---
// 900 × 4s = 3600s = 1 hour | 900 × 20B × 2 slaves = 36KB
#define HISTORY_MAX        900
#define HISTORY_INTERVAL_S 4       // Record every 4 seconds

// --- Serial ---
#define SERIAL_BAUD        115200

#endif

// ============================================
// FILE: globals.h
// ============================================

#ifndef GLOBALS_H
#define GLOBALS_H


extern SlaveData    slaves[NUM_SLAVES];
extern BaselineData baselines[NUM_SLAVES];
extern unsigned long cycleCount;
extern unsigned long globalSeqNum;
extern bool baselineComplete;

extern String apIP;
extern String staIP;
extern bool ntpSynced;

extern Preferences prefs;

// Recalibrate
extern volatile bool needRecalibrate;
extern bool calibDone;
extern int calibAckCount;
extern String calibResult;

// History ring buffer (1 hour, auto-overwrite)
extern HistoryPoint history[NUM_SLAVES][HISTORY_MAX];
extern int historyHead[NUM_SLAVES];
extern int historyCount[NUM_SLAVES];
extern unsigned long lastHistoryTime;

#endif

// ============================================
// FILE: baseline.h
// ============================================

#ifndef BASELINE_H
#define BASELINE_H

void saveBaseline();
void loadBaseline();
void resetBaselineData();
void updateBaseline(int slaveIdx);
int  checkAnomaly(int slaveIdx);

#endif

// ============================================
// FILE: dashboard.h
// ============================================

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

// ============================================
// FILE: data_types.h
// ============================================

#ifndef DATA_TYPES_H
#define DATA_TYPES_H


struct SlaveData {
    // Z-axis (vertical)
    float topFreqsZ[NUM_TOP_FREQS];
    float rmsZ, peakZ, crestFactorZ;
    // X-axis (horizontal 1)
    float freqX, rmsX, peakX;
    // Y-axis (horizontal 2)
    float freqY, rmsY, peakY;
    // Alert
    int   slaveAlert, masterAlert, finalAlert;
    String alertReasons;
    // Connectivity
    int   rssi;
    bool  online;
    unsigned long lastSeen;
    // Packet statistics
    unsigned long totalPolls, successPolls;
    unsigned long lastSeqSent, lastSeqRecv;
};

struct BaselineData {
    float f1_history[BASELINE_CYCLES];
    float rms_history[BASELINE_CYCLES];
    float peak_history[BASELINE_CYCLES];
    int   count;
    bool  ready;
    float f1_mean, f1_std;
    float rms_mean, rms_std;
    float peak_mean, peak_std;
};

// 20 bytes per point — compact for ring buffer
struct HistoryPoint {
    uint32_t timeSec;       // Uptime in seconds
    float    rmsZ;          // RMS vibration (g)
    float    peakZ;         // Peak vibration (g)
    float    freqZ;         // Dominant frequency (Hz)
    uint8_t  alert;         // Alert level (0/1/2)
    uint8_t  _pad[3];       // Alignment padding
};

#endif

// ============================================
// FILE: lora_comm.h
// ============================================

#ifndef LORA_COMM_H
#define LORA_COMM_H


bool pollSlave(uint8_t slaveId);
void performRecalibrate();
void printResults();

#endif

// ============================================
// FILE: web_server.h
// ============================================

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

void setupWebServer();

#endif

// ============================================
// FILE: wifi_manager.h
// ============================================

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H


void setupWiFi();
void setupNTP();
String getTimeString();

#endif

// ============================================
// FILE: globals.cpp
// ============================================


SlaveData    slaves[NUM_SLAVES];
BaselineData baselines[NUM_SLAVES];
unsigned long cycleCount = 0;
unsigned long globalSeqNum = 0;
bool baselineComplete = false;

String apIP = "";
String staIP = "";
bool ntpSynced = false;

Preferences prefs;

volatile bool needRecalibrate = false;
bool calibDone = false;
int calibAckCount = 0;
String calibResult = "";

HistoryPoint history[NUM_SLAVES][HISTORY_MAX];
int historyHead[NUM_SLAVES] = {0};
int historyCount[NUM_SLAVES] = {0};
unsigned long lastHistoryTime = 0;

// ============================================
// FILE: baseline.cpp
// ============================================


// ============================================
// NVS Persistence
// ============================================

void saveBaseline() {
    prefs.begin("baseline", false);
    prefs.putBool("complete", baselineComplete);
    for (int i = 0; i < NUM_SLAVES; i++) {
        String p = "s" + String(i);
        prefs.putBool((p + "r").c_str(), baselines[i].ready);
        prefs.putInt((p + "c").c_str(), baselines[i].count);
        prefs.putFloat((p + "fm").c_str(), baselines[i].f1_mean);
        prefs.putFloat((p + "fs").c_str(), baselines[i].f1_std);
        prefs.putFloat((p + "rm").c_str(), baselines[i].rms_mean);
        prefs.putFloat((p + "rs").c_str(), baselines[i].rms_std);
        prefs.putFloat((p + "pm").c_str(), baselines[i].peak_mean);
        prefs.putFloat((p + "ps").c_str(), baselines[i].peak_std);
    }
    prefs.end();
    Serial.println("[NVS] Baseline saved");
}

void loadBaseline() {
    prefs.begin("baseline", true);
    if (prefs.getBool("complete", false)) {
        baselineComplete = true;
        for (int i = 0; i < NUM_SLAVES; i++) {
            String p = "s" + String(i);
            baselines[i].ready     = prefs.getBool((p + "r").c_str(), false);
            baselines[i].count     = prefs.getInt((p + "c").c_str(), 0);
            baselines[i].f1_mean   = prefs.getFloat((p + "fm").c_str(), 0);
            baselines[i].f1_std    = prefs.getFloat((p + "fs").c_str(), 0.01f);
            baselines[i].rms_mean  = prefs.getFloat((p + "rm").c_str(), 0);
            baselines[i].rms_std   = prefs.getFloat((p + "rs").c_str(), 0.0001f);
            baselines[i].peak_mean = prefs.getFloat((p + "pm").c_str(), 0);
            baselines[i].peak_std  = prefs.getFloat((p + "ps").c_str(), 0.0001f);
        }
        Serial.println("[NVS] Baseline loaded, anomaly detection ACTIVE");
    } else {
        Serial.println("[NVS] No saved baseline");
    }
    prefs.end();
}

void resetBaselineData() {
    baselineComplete = false;
    for (int i = 0; i < NUM_SLAVES; i++) {
        baselines[i].count = 0;
        baselines[i].ready = false;
        // Clear stale alerts from previous baseline
        slaves[i].masterAlert = ALERT_NORMAL;
        slaves[i].finalAlert = slaves[i].slaveAlert;
        slaves[i].alertReasons = "";
    }
    prefs.begin("baseline", false);
    prefs.clear();
    prefs.end();
    Serial.println("[NVS] Baseline RESET");
}

// ============================================
// Baseline Learning
// ============================================

void updateBaseline(int slaveIdx) {
    BaselineData& b = baselines[slaveIdx];
    SlaveData& s = slaves[slaveIdx];
    if (b.ready || !s.online) return;

    int idx = b.count;
    if (idx < BASELINE_CYCLES) {
        b.f1_history[idx]   = s.topFreqsZ[0];
        b.rms_history[idx]  = s.rmsZ;
        b.peak_history[idx] = s.peakZ;
        b.count++;
    }

    if (b.count >= BASELINE_CYCLES) {
        // Compute mean
        b.f1_mean = 0; b.rms_mean = 0; b.peak_mean = 0;
        for (int i = 0; i < BASELINE_CYCLES; i++) {
            b.f1_mean   += b.f1_history[i];
            b.rms_mean  += b.rms_history[i];
            b.peak_mean += b.peak_history[i];
        }
        b.f1_mean /= BASELINE_CYCLES;
        b.rms_mean /= BASELINE_CYCLES;
        b.peak_mean /= BASELINE_CYCLES;

        // Compute std deviation
        b.f1_std = 0; b.rms_std = 0; b.peak_std = 0;
        for (int i = 0; i < BASELINE_CYCLES; i++) {
            float d;
            d = b.f1_history[i] - b.f1_mean;     b.f1_std += d * d;
            d = b.rms_history[i] - b.rms_mean;    b.rms_std += d * d;
            d = b.peak_history[i] - b.peak_mean;  b.peak_std += d * d;
        }
        b.f1_std   = sqrtf(b.f1_std / BASELINE_CYCLES);
        b.rms_std  = sqrtf(b.rms_std / BASELINE_CYCLES);
        b.peak_std = sqrtf(b.peak_std / BASELINE_CYCLES);

        // Clamp minimum std to avoid division by zero
        if (b.f1_std < 0.01f)     b.f1_std = 0.01f;
        if (b.rms_std < 0.0001f)  b.rms_std = 0.0001f;
        if (b.peak_std < 0.0001f) b.peak_std = 0.0001f;

        b.ready = true;
        Serial.print("[BASELINE] S");
        Serial.print(slaveIdx + 1);
        Serial.print(" COMPLETE: f1=");
        Serial.print(b.f1_mean, 2);
        Serial.print(" +/- ");
        Serial.print(b.f1_std, 2);
        Serial.println(" Hz");
    }
}

// ============================================
// 3-Sigma Anomaly Detection
// ============================================

int checkAnomaly(int slaveIdx) {
    BaselineData& b = baselines[slaveIdx];
    SlaveData& s = slaves[slaveIdx];
    if (!b.ready || !s.online) return ALERT_NORMAL;

    int alert = ALERT_NORMAL;
    s.alertReasons = "";

    // Frequency shift check
    float f1_dev = fabsf(s.topFreqsZ[0] - b.f1_mean) / b.f1_std;
    if (f1_dev > ANOMALY_SIGMA * 2) {
        alert = ALERT_CRITICAL;
        s.alertReasons += "FREQ_SHIFT(" + String(f1_dev, 1) + "s) ";
    } else if (f1_dev > ANOMALY_SIGMA) {
        alert = max(alert, ALERT_WARNING);
        s.alertReasons += "freq(" + String(f1_dev, 1) + "s) ";
    }

    // RMS vibration check
    float rms_dev = fabsf(s.rmsZ - b.rms_mean) / b.rms_std;
    if (rms_dev > ANOMALY_SIGMA * 2) {
        alert = max(alert, ALERT_CRITICAL);
        s.alertReasons += "HIGH_VIB(" + String(rms_dev, 1) + "s) ";
    } else if (rms_dev > ANOMALY_SIGMA) {
        alert = max(alert, ALERT_WARNING);
        s.alertReasons += "vib(" + String(rms_dev, 1) + "s) ";
    }

    // Peak impact check
    float peak_dev = fabsf(s.peakZ - b.peak_mean) / b.peak_std;
    if (peak_dev > ANOMALY_SIGMA * 2) {
        alert = max(alert, ALERT_CRITICAL);
        s.alertReasons += "IMPACT(" + String(peak_dev, 1) + "s) ";
    } else if (peak_dev > ANOMALY_SIGMA) {
        alert = max(alert, ALERT_WARNING);
        s.alertReasons += "impact(" + String(peak_dev, 1) + "s) ";
    }

    return alert;
}

// ============================================
// FILE: lora_comm.cpp
// ============================================


// ============================================
// Poll Slave (with retry)
// ============================================

bool pollSlave(uint8_t slaveId) {
    int idx = slaveId - 1;
    slaves[idx].totalPolls++;

    for (int attempt = 0; attempt <= MAX_RETRIES; attempt++) {
        if (attempt > 0) {
            Serial.print("  Retry #");
            Serial.print(attempt);
            Serial.print("... ");
            delay(RETRY_DELAY_MS);
        }

        globalSeqNum++;
        String req = "REQ:" + String(slaveId) + ":" + String(globalSeqNum);
        slaves[idx].lastSeqSent = globalSeqNum;

        LoRa.beginPacket();
        LoRa.print(req);
        LoRa.endPacket();
        LoRa.receive();

        unsigned long startTime = millis();
        while (millis() - startTime < POLL_TIMEOUT_MS) {
            int packetSize = LoRa.parsePacket();
            if (packetSize > 0) {
                String response = "";
                while (LoRa.available()) {
                    response += (char)LoRa.read();
                }
                int rssi = LoRa.packetRssi();

                if (response.startsWith("DATA:")) {
                    String data = response.substring(5);

                    // Count commas to determine protocol version
                    int commas[14];
                    int commaCount = 0;
                    for (int i = 0; i < (int)data.length() && commaCount < 14; i++) {
                        if (data.charAt(i) == ',') {
                            commas[commaCount++] = i;
                        }
                    }

                    if (commaCount >= 7) {
                        int id = data.substring(0, commas[0]).toInt();

                        if (id == slaveId) {
                            if (commaCount == 14) {
                                // v3.2: id,seq,f1z,f2z,f3z,rmsZ,peakZ,cfZ,alert,f1x,rmsX,peakX,f1y,rmsY,peakY
                                unsigned long respSeq = data.substring(commas[0] + 1, commas[1]).toInt();
                                if (respSeq != globalSeqNum) continue; // Reject stale packet
                                slaves[idx].lastSeqRecv     = respSeq;
                                slaves[idx].topFreqsZ[0]    = data.substring(commas[1] + 1, commas[2]).toFloat();
                                slaves[idx].topFreqsZ[1]    = data.substring(commas[2] + 1, commas[3]).toFloat();
                                slaves[idx].topFreqsZ[2]    = data.substring(commas[3] + 1, commas[4]).toFloat();
                                slaves[idx].rmsZ            = data.substring(commas[4] + 1, commas[5]).toFloat();
                                slaves[idx].peakZ           = data.substring(commas[5] + 1, commas[6]).toFloat();
                                slaves[idx].crestFactorZ    = data.substring(commas[6] + 1, commas[7]).toFloat();
                                slaves[idx].slaveAlert      = data.substring(commas[7] + 1, commas[8]).toInt();
                                slaves[idx].freqX           = data.substring(commas[8] + 1, commas[9]).toFloat();
                                slaves[idx].rmsX            = data.substring(commas[9] + 1, commas[10]).toFloat();
                                slaves[idx].peakX           = data.substring(commas[10] + 1, commas[11]).toFloat();
                                slaves[idx].freqY           = data.substring(commas[11] + 1, commas[12]).toFloat();
                                slaves[idx].rmsY            = data.substring(commas[12] + 1, commas[13]).toFloat();
                                slaves[idx].peakY           = data.substring(commas[13] + 1).toFloat();
                            } else if (commaCount == 10) {
                                // v3.0: id,seq,f1z,f2z,f3z,rmsZ,peakZ,cfZ,alert,rmsX,rmsY
                                unsigned long respSeq = data.substring(commas[0] + 1, commas[1]).toInt();
                                if (respSeq != globalSeqNum) continue; // Reject stale packet
                                slaves[idx].lastSeqRecv     = respSeq;
                                slaves[idx].topFreqsZ[0]    = data.substring(commas[1] + 1, commas[2]).toFloat();
                                slaves[idx].topFreqsZ[1]    = data.substring(commas[2] + 1, commas[3]).toFloat();
                                slaves[idx].topFreqsZ[2]    = data.substring(commas[3] + 1, commas[4]).toFloat();
                                slaves[idx].rmsZ            = data.substring(commas[4] + 1, commas[5]).toFloat();
                                slaves[idx].peakZ           = data.substring(commas[5] + 1, commas[6]).toFloat();
                                slaves[idx].crestFactorZ    = data.substring(commas[6] + 1, commas[7]).toFloat();
                                slaves[idx].slaveAlert      = data.substring(commas[7] + 1, commas[8]).toInt();
                                slaves[idx].rmsX            = data.substring(commas[8] + 1, commas[9]).toFloat();
                                slaves[idx].rmsY            = data.substring(commas[9] + 1).toFloat();
                                slaves[idx].freqX = 0; slaves[idx].peakX = 0;
                                slaves[idx].freqY = 0; slaves[idx].peakY = 0;
                            } else {
                                // v2.0: id,f1z,f2z,f3z,rmsZ,peakZ,cfZ,alert
                                slaves[idx].topFreqsZ[0]    = data.substring(commas[0] + 1, commas[1]).toFloat();
                                slaves[idx].topFreqsZ[1]    = data.substring(commas[1] + 1, commas[2]).toFloat();
                                slaves[idx].topFreqsZ[2]    = data.substring(commas[2] + 1, commas[3]).toFloat();
                                slaves[idx].rmsZ            = data.substring(commas[3] + 1, commas[4]).toFloat();
                                slaves[idx].peakZ           = data.substring(commas[4] + 1, commas[5]).toFloat();
                                slaves[idx].crestFactorZ    = data.substring(commas[5] + 1, commas[6]).toFloat();
                                slaves[idx].slaveAlert      = data.substring(commas[6] + 1).toInt();
                                slaves[idx].rmsX = 0; slaves[idx].peakX = 0; slaves[idx].freqX = 0;
                                slaves[idx].rmsY = 0; slaves[idx].peakY = 0; slaves[idx].freqY = 0;
                            }

                            slaves[idx].rssi     = rssi;
                            slaves[idx].online   = true;
                            slaves[idx].lastSeen = millis();
                            slaves[idx].successPolls++;
                            return true;
                        }
                    }
                }
            }
            delay(1);
        }
    }

    // Clear stale state when slave goes offline
    slaves[slaveId - 1].online = false;
    slaves[slaveId - 1].slaveAlert = ALERT_NORMAL;
    slaves[slaveId - 1].masterAlert = ALERT_NORMAL;
    slaves[slaveId - 1].finalAlert = ALERT_NORMAL;
    slaves[slaveId - 1].alertReasons = "";
    return false;
}

// ============================================
// Sequential Recalibrate (WDT-safe)
// ============================================

void performRecalibrate() {
    Serial.println("[CMD] Recalibrating all slaves...");
    calibResult = "";
    calibAckCount = 0;

    for (int s = 1; s <= NUM_SLAVES; s++) {
        bool gotAck = false;

        for (int attempt = 0; attempt < 3 && !gotAck; attempt++) {
            if (attempt > 0) {
                Serial.print("[CMD] Retry S"); Serial.print(s);
                Serial.print(" #"); Serial.println(attempt);
                delay(500);
            }

            String cmd = "CALIB:" + String(s);
            LoRa.beginPacket();
            LoRa.print(cmd);
            LoRa.endPacket();
            LoRa.receive();

            Serial.print("[CMD] Sent "); Serial.print(cmd);
            Serial.print(" > waiting... ");

            unsigned long start = millis();
            while (millis() - start < 4000) {
                yield();  // Feed WDT
                int ps = LoRa.parsePacket();
                if (ps > 0) {
                    String msg = "";
                    while (LoRa.available()) msg += (char)LoRa.read();
                    if (msg.startsWith("ACK:")) {
                        int ackId = msg.substring(4, msg.indexOf(',')).toInt();
                        if (ackId == s) {
                            gotAck = true;
                            calibAckCount++;
                            if (calibResult.length() > 0) calibResult += " | ";
                            calibResult += "S" + String(s) + ":" + msg.substring(msg.lastIndexOf(',') + 1);
                            Serial.print("OK ("); Serial.print(msg); Serial.println(")");
                            break;
                        }
                    }
                }
                delay(10);
            }

            if (!gotAck && attempt < 2) Serial.println("TIMEOUT");
        }

        if (!gotAck) {
            Serial.print("[CMD] S"); Serial.print(s); Serial.println(" NO RESPONSE");
            if (calibResult.length() > 0) calibResult += " | ";
            calibResult += "S" + String(s) + ":FAIL";
        }

        if (s < NUM_SLAVES) delay(200);
        yield();
    }

    calibDone = true;
    needRecalibrate = false;
    Serial.print("[CMD] Done: "); Serial.print(calibAckCount);
    Serial.print("/"); Serial.print(NUM_SLAVES); Serial.println(" OK");
}

// ============================================
// Serial Monitor Output
// ============================================

void printResults() {
    int onlineCount = 0, warningCount = 0, criticalCount = 0;
    bool hasAlerts = false;

    Serial.println();
    Serial.println("========================================================================");
    Serial.print("  BRIDGE MONITOR v3.2 [Cycle #");
    Serial.print(cycleCount);
    Serial.print("]  Up: ");
    Serial.print(millis() / 1000);
    Serial.print("s");
    String t = getTimeString();
    if (t.length() > 0) { Serial.print("  | "); Serial.print(t); }
    Serial.println();

    Serial.print("  Mode: ");
    if (!baselineComplete) {
        int mc = BASELINE_CYCLES;
        for (int i = 0; i < NUM_SLAVES; i++) {
            if (baselines[i].count < mc) mc = baselines[i].count;
        }
        Serial.print("LEARNING ("); Serial.print(mc); Serial.print("/");
        Serial.print(BASELINE_CYCLES); Serial.println(")");
    } else {
        Serial.println("MONITORING");
    }
    Serial.print("  Web: http://"); Serial.print(apIP);
    if (staIP.length() > 0) { Serial.print(" | http://"); Serial.print(staIP); }
    Serial.println();
    Serial.println("------------------------------------------------------------------------");

    for (int i = 0; i < NUM_SLAVES; i++) {
        Serial.print("  Slave "); Serial.print(i + 1); Serial.print(": ");

        if (slaves[i].online) {
            onlineCount++;
            Serial.print("Z: f1="); Serial.print(slaves[i].topFreqsZ[0], 2);
            Serial.print("  f2="); Serial.print(slaves[i].topFreqsZ[1], 2);
            Serial.print("  f3="); Serial.print(slaves[i].topFreqsZ[2], 2);
            Serial.println(" Hz");

            Serial.print("          Z: RMS="); Serial.print(slaves[i].rmsZ, 4);
            Serial.print(" | Peak="); Serial.print(slaves[i].peakZ, 4);
            Serial.print(" | CF="); Serial.print(slaves[i].crestFactorZ, 2);
            Serial.print(" | "); Serial.print(slaves[i].rssi); Serial.println(" dBm");

            Serial.print("          X: f1="); Serial.print(slaves[i].freqX, 2);
            Serial.print(" Hz | RMS="); Serial.print(slaves[i].rmsX, 4);
            Serial.print(" | Peak="); Serial.print(slaves[i].peakX, 4); Serial.println(" g");

            Serial.print("          Y: f1="); Serial.print(slaves[i].freqY, 2);
            Serial.print(" Hz | RMS="); Serial.print(slaves[i].rmsY, 4);
            Serial.print(" | Peak="); Serial.print(slaves[i].peakY, 4); Serial.println(" g");

            Serial.print("          Pkt: ");
            Serial.print(slaves[i].successPolls); Serial.print("/");
            Serial.print(slaves[i].totalPolls);
            if (slaves[i].totalPolls > 0) {
                Serial.print(" ("); Serial.print(slaves[i].successPolls * 100 / slaves[i].totalPolls);
                Serial.print("%)");
            }
            Serial.println();

            Serial.print("          ");
            if (slaves[i].finalAlert == ALERT_CRITICAL) {
                Serial.print("!!! CRITICAL !!! "); criticalCount++; hasAlerts = true;
            } else if (slaves[i].finalAlert == ALERT_WARNING) {
                Serial.print(">> WARNING << "); warningCount++; hasAlerts = true;
            } else {
                Serial.print("[OK] NORMAL ");
            }
            if (slaves[i].alertReasons.length() > 0) {
                Serial.print("| "); Serial.print(slaves[i].alertReasons);
            }
            Serial.println();
        } else {
            unsigned long off = slaves[i].lastSeen > 0 ? (millis() - slaves[i].lastSeen) / 1000 : 0;
            Serial.print("--- OFFLINE ---");
            if (off > 0) { Serial.print(" ("); Serial.print(off); Serial.print("s ago)"); }
            Serial.println();
        }
        if (i < NUM_SLAVES - 1) Serial.println();
    }

    Serial.println("------------------------------------------------------------------------");
    Serial.print("  "); Serial.print(onlineCount); Serial.print("/");
    Serial.print(NUM_SLAVES); Serial.print(" ONLINE");
    if (criticalCount > 0) { Serial.print(" | "); Serial.print(criticalCount); Serial.print(" CRIT"); }
    if (warningCount > 0) { Serial.print(" | "); Serial.print(warningCount); Serial.print(" WARN"); }
    if (!hasAlerts && onlineCount == NUM_SLAVES) Serial.print("  * ALL NORMAL *");
    Serial.println();
    Serial.println("========================================================================");
}

// ============================================
// FILE: web_server.cpp
// ============================================


static AsyncWebServer server(80);

// Escape special chars in a string for safe JSON embedding
static String jsonEscape(const String& s) {
    String out;
    out.reserve(s.length() + 4);
    for (unsigned int i = 0; i < s.length(); i++) {
        char c = s.charAt(i);
        if (c == '"')       out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else                out += c;
    }
    return out;
}

void setupWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", DASHBOARD_HTML);
    });

    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Snapshot shared state to avoid torn reads from async context
        SlaveData snap[NUM_SLAVES];
        int snapBaseCount[NUM_SLAVES];
        for (int i = 0; i < NUM_SLAVES; i++) {
            snap[i] = slaves[i];
            snapBaseCount[i] = baselines[i].count;
        }
        unsigned long snapCycle = cycleCount;
        bool snapBaseComplete = baselineComplete;
        String snapCalibResult = calibResult;

        String json = "{";
        json += "\"cycle\":" + String(snapCycle) + ",";
        json += "\"uptime\":" + String(millis() / 1000) + ",";
        json += "\"baselineReady\":" + String(snapBaseComplete ? "true" : "false") + ",";

        int minCount = BASELINE_CYCLES;
        for (int i = 0; i < NUM_SLAVES; i++) {
            if (snapBaseCount[i] < minCount) minCount = snapBaseCount[i];
        }
        json += "\"baselineProgress\":" + String(minCount) + ",";
        json += "\"ntpTime\":\"" + getTimeString() + "\",";
        json += "\"apIP\":\"" + apIP + "\",";
        json += "\"staIP\":\"" + staIP + "\",";

        unsigned long totalP = 0, successP = 0;
        for (int i = 0; i < NUM_SLAVES; i++) {
            totalP += snap[i].totalPolls;
            successP += snap[i].successPolls;
        }
        json += "\"totalPolls\":" + String(totalP) + ",";
        json += "\"successPolls\":" + String(successP) + ",";

        json += "\"slaves\":[";
        for (int i = 0; i < NUM_SLAVES; i++) {
            if (i > 0) json += ",";
            json += "{";
            json += "\"id\":" + String(i + 1) + ",";
            json += "\"online\":" + String(snap[i].online ? "true" : "false") + ",";
            json += "\"f1z\":" + String(snap[i].topFreqsZ[0], 2) + ",";
            json += "\"f2z\":" + String(snap[i].topFreqsZ[1], 2) + ",";
            json += "\"f3z\":" + String(snap[i].topFreqsZ[2], 2) + ",";
            json += "\"rmsZ\":" + String(snap[i].rmsZ, 4) + ",";
            json += "\"peakZ\":" + String(snap[i].peakZ, 4) + ",";
            json += "\"cfZ\":" + String(snap[i].crestFactorZ, 2) + ",";
            json += "\"freqX\":" + String(snap[i].freqX, 2) + ",";
            json += "\"rmsX\":" + String(snap[i].rmsX, 4) + ",";
            json += "\"peakX\":" + String(snap[i].peakX, 4) + ",";
            json += "\"freqY\":" + String(snap[i].freqY, 2) + ",";
            json += "\"rmsY\":" + String(snap[i].rmsY, 4) + ",";
            json += "\"peakY\":" + String(snap[i].peakY, 4) + ",";
            json += "\"alert\":" + String(snap[i].finalAlert) + ",";
            json += "\"rssi\":" + String(snap[i].rssi) + ",";
            json += "\"alertReasons\":\"" + jsonEscape(snap[i].alertReasons) + "\",";
            unsigned long offSec = 0;
            if (!snap[i].online && snap[i].lastSeen > 0) {
                offSec = (millis() - snap[i].lastSeen) / 1000;
            }
            json += "\"lastSeen\":" + String(offSec);
            json += "}";
        }
        json += "]}";
        request->send(200, "application/json", json);
    });

    server.on("/api/reset-baseline", HTTP_POST, [](AsyncWebServerRequest *request) {
        resetBaselineData();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/api/recalibrate", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (needRecalibrate) {
            request->send(200, "application/json", "{\"status\":\"busy\"}");
            return;
        }
        needRecalibrate = true;
        calibDone = false;
        request->send(200, "application/json", "{\"status\":\"started\"}");
        Serial.println("[CMD] Recalibrate requested via dashboard");
    });

    server.on("/api/recalibrate-status", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Snapshot calibResult to avoid torn String read
        String snapResult = calibResult;
        String json = "{\"done\":" + String(calibDone ? "true" : "false") +
                      ",\"acks\":" + String(calibAckCount) +
                      ",\"total\":" + String(NUM_SLAVES) +
                      ",\"detail\":\"" + jsonEscape(snapResult) + "\"}";
        request->send(200, "application/json", json);
    });

    // History API: /api/history?slave=0&since=0
    // - First call (since=0): returns ALL points
    // - Subsequent (since=lastTimeSec): returns only NEW points
    server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *request) {
        int si = 0;
        unsigned long since = 0;
        if (request->hasParam("slave")) {
            si = request->getParam("slave")->value().toInt();
        }
        if (request->hasParam("since")) {
            since = request->getParam("since")->value().toInt();
        }
        if (si < 0 || si >= NUM_SLAVES) si = 0;

        int count = historyCount[si];
        int head  = historyHead[si];

        String json = "{\"slave\":" + String(si + 1) +
                      ",\"count\":" + String(count) +
                      ",\"interval\":" + String(HISTORY_INTERVAL_S) +
                      ",\"max\":" + String(HISTORY_MAX) +
                      ",\"points\":[";

        int start = (count < HISTORY_MAX) ? 0 : head;
        bool first = true;
        int sent = 0;
        for (int k = 0; k < count; k++) {
            int idx = (start + k) % HISTORY_MAX;
            HistoryPoint& pt = history[si][idx];
            if (pt.timeSec <= since) continue; // Skip already-sent points
            if (!first) json += ",";
            json += "[" + String(pt.timeSec) + "," +
                    String(pt.rmsZ, 4) + "," +
                    String(pt.peakZ, 4) + "," +
                    String(pt.freqZ, 2) + "," +
                    String(pt.alert) + "]";
            first = false;
            sent++;
        }
        json += "],\"sent\":" + String(sent) + "}";
        request->send(200, "application/json", json);
    });

    server.begin();
    Serial.println("[WEB] Server started on port 80");
}

// ============================================
// FILE: wifi_manager.cpp
// ============================================


void setupWiFi() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
    delay(500);
    apIP = WiFi.softAPIP().toString();
    Serial.print("[WIFI] AP: ");
    Serial.print(WIFI_AP_SSID);
    Serial.print(" > http://");
    Serial.println(apIP);

    String ssid = WIFI_STA_SSID;
    if (ssid.length() > 0) {
        Serial.print("[WIFI] STA > ");
        Serial.print(WIFI_STA_SSID);
        WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            staIP = WiFi.localIP().toString();
            Serial.print(" OK > http://");
            Serial.println(staIP);
        } else {
            Serial.println(" FAILED (AP-only)");
            WiFi.mode(WIFI_AP);
            WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
            delay(500);
            apIP = WiFi.softAPIP().toString();
        }
    } else {
        Serial.println("[WIFI] STA not configured, AP-only");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
        delay(500);
        apIP = WiFi.softAPIP().toString();
    }
}

void setupNTP() {
    if (WiFi.status() == WL_CONNECTED) {
        configTime(NTP_GMT_OFFSET, NTP_DAYLIGHT, NTP_SERVER);
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 5000)) {
            ntpSynced = true;
            Serial.print("[NTP] OK: ");
            Serial.println(getTimeString());
        } else {
            Serial.println("[NTP] FAILED");
        }
    }
}

String getTimeString() {
    struct tm timeinfo;
    if (ntpSynced && getLocalTime(&timeinfo, 100)) {
        char buf[20];
        strftime(buf, sizeof(buf), "%H:%M:%S %d/%m", &timeinfo);
        return String(buf);
    }
    return "";
}

// ============================================
// FILE: main.cpp
// ============================================

/**
 * ============================================
 * MASTER ESP32 — Bridge Vibration Monitor v3.2
 * ============================================
 *
 * Architecture: Always-On polling + Web Dashboard
 * - Polls 2 slaves via LoRa every ~1s
 * - Baseline learning + 3-sigma anomaly detection
 * - WiFi AP+STA with async web dashboard
 */


void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  BRIDGE VIBRATION MONITOR v3.2 MASTER");
    Serial.println("  Always-On + Multi-Axis + Dashboard");
    Serial.println("========================================");
    Serial.println();

    // LoRa
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    Serial.print("[INIT] LoRa 433MHz... ");
    if (!LoRa.begin(LORA_FREQ)) {
        Serial.println("FAILED!");
        while (1) { delay(1000); }
    }
    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setCodingRate4(LORA_CR);
    LoRa.setTxPower(LORA_TX_POWER);
    LoRa.enableCrc();
    Serial.println("OK");

    // Init slave data
    for (int i = 0; i < NUM_SLAVES; i++) {
        slaves[i] = {};
        baselines[i] = {};
    }

    // Network + Web
    setupWiFi();
    setupNTP();
    setupWebServer();
    loadBaseline();

    Serial.println("[INIT] Ready");
    Serial.println();
}

void loop() {
    // Handle recalibrate request (non-blocking, WDT-safe)
    if (needRecalibrate) {
        performRecalibrate();
        return;
    }

    cycleCount++;

    // Poll all slaves
    for (int i = 1; i <= NUM_SLAVES; i++) {
        Serial.print("  Poll S"); Serial.print(i); Serial.print("... ");
        if (pollSlave(i)) {
            Serial.print("OK (Z:");
            Serial.print(slaves[i - 1].topFreqsZ[0], 1);
            Serial.print(" X:");
            Serial.print(slaves[i - 1].freqX, 1);
            Serial.print(" Y:");
            Serial.print(slaves[i - 1].freqY, 1);
            Serial.println(" Hz)");
        } else {
            Serial.println("FAIL");
        }
        if (i < NUM_SLAVES) delay(GUARD_TIME_MS);
    }

    // Baseline learning
    for (int i = 0; i < NUM_SLAVES; i++) updateBaseline(i);

    if (!baselineComplete) {
        bool allReady = true;
        for (int i = 0; i < NUM_SLAVES; i++) {
            if (!baselines[i].ready) { allReady = false; break; }
        }
        if (allReady) {
            baselineComplete = true;
            saveBaseline();
            Serial.println("* BASELINE COMPLETE — Anomaly detection ACTIVE *");
        }
    }

    // Anomaly detection
    for (int i = 0; i < NUM_SLAVES; i++) {
        if (slaves[i].online) {
            slaves[i].masterAlert = checkAnomaly(i);
            slaves[i].finalAlert = max(slaves[i].slaveAlert, slaves[i].masterAlert);
        }
    }

    // Record history (ring buffer, auto-overwrites after HISTORY_MAX)
    unsigned long nowSec = millis() / 1000;
    if (nowSec - lastHistoryTime >= HISTORY_INTERVAL_S) {
        lastHistoryTime = nowSec;
        for (int i = 0; i < NUM_SLAVES; i++) {
            if (slaves[i].online) {
                HistoryPoint& pt = history[i][historyHead[i]];
                pt.timeSec = nowSec;
                pt.rmsZ    = slaves[i].rmsZ;
                pt.peakZ   = slaves[i].peakZ;
                pt.freqZ   = slaves[i].topFreqsZ[0];
                pt.alert   = (uint8_t)slaves[i].finalAlert;
                historyHead[i] = (historyHead[i] + 1) % HISTORY_MAX;
                if (historyCount[i] < HISTORY_MAX) historyCount[i]++;
            }
        }
    }

    printResults();
    delay(POLL_INTERVAL_MS);
}
