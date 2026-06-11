#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

#define EEPROM_SIZE 64

const char* ssid = "Test";
const char* password = "asdfghjkl";

WebServer server(80);

// Pins
int sensorPins[4] = {34,33,32,35};
int ledPins[4] = {15,4,17,18};

int v[4];

// Calibration
float blood_th[4];
float noblood_th[4];

bool calibrating=false;
int mode=0;
unsigned long startTime;
long sum[4];
int count=0;

unsigned long lastRead=0;

// ================= HTML =================
const char page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Bubble Dashboard</title>

<style>
body{margin:0;font-family:Segoe UI;background:#0f2027;color:white;}
.wrapper{display:flex;}
.sidebar{width:260px;background:#111;padding:15px;}
.main{flex:1;text-align:center;}

.card{width:120px;margin:10px;padding:15px;border-radius:15px;display:inline-block;}
.noblood{background:#1e88e5;}
.blood{background:#e53935;}

canvas{background:white;margin-top:15px;border-radius:10px;}
input{width:70px;padding:5px;margin:5px;}
button{padding:6px;margin:5px;}

table{border-collapse:collapse;margin:15px auto;background:white;color:#222;min-width:520px;width:90%;max-width:720px;}
table th,table td{border:1px solid #bbb;padding:8px 14px;text-align:center;font-weight:normal;width:16.66%;}
table th{background:#222;color:white;font-weight:bold;position:sticky;top:0;z-index:2;}
.bubble{background:#e53935;color:white;font-weight:bold;}
.thresholdRow td{background:#f0f0f0;font-weight:bold;color:#222;}
.countCol{background:#222;color:#fff;font-weight:bold;}
.countOk{background:#43a047;color:#fff;}

.tabBar{display:flex;justify-content:center;margin:10px 0 20px 0;}
.tabBar button{background:#222;color:white;border:1px solid #444;border-bottom:none;border-radius:8px 8px 0 0;padding:8px 22px;cursor:pointer;font-size:15px;}
.tabBar button.active{background:#1e88e5;border-color:#1e88e5;}
.tabContent{display:none;}
.tabContent.active{display:block;}

.tableScroll{max-height:520px;overflow-y:auto;margin:0 auto;width:90%;max-width:720px;border:1px solid #bbb;border-radius:6px;}
.tableScroll table{margin:0;width:100%;}
</style>
</head>

<body>

<div class="wrapper">

<div class="sidebar">
<h3>Calibration</h3>
<b>Blood:</b><div id="bloodData"></div><br>
<b>No Blood:</b><div id="noBloodData"></div><br>
<b>Threshold:</b><div id="thresholdData"></div>
</div>

<div class="main">

<h2>Bubble Detection Dashboard</h2>

<div class="tabBar">
<button id="tabBtnDash" class="active" onclick="showTab('dash')">Dashboard</button>
<button id="tabBtnTable" onclick="showTab('table')">Table</button>
</div>

<div id="tabDash" class="tabContent active">
<div>
<div id="c1" class="card"><div>S1</div><div id="v1">0</div></div>
<div id="c2" class="card"><div>S2</div><div id="v2">0</div></div>
<div id="c3" class="card"><div>S3</div><div id="v3">0</div></div>
<div id="c4" class="card"><div>S4</div><div id="v4">0</div></div>
</div>

<button onclick="cal(1)">WITH Blood</button>
<button onclick="cal(2)">WITHOUT Blood</button>

<p id="timer"></p>

<h3>All Sensors</h3>
Min<input id="gmin" value="0"> Max<input id="gmax" value="4095">
<button onclick="setRange()">Apply</button>
<canvas id="gAll" width="900" height="250"></canvas>

<div id="graphs"></div>
</div>

<div id="tabTable" class="tabContent">
<h3>Live Sensor Table</h3>
<div class="tableScroll">
<table id="dataTable">
<thead>
<tr>
<th>Count</th>
<th>S1</th>
<th>S2</th>
<th>S3</th>
<th>S4</th>
<th>Label</th>
</tr>
<tr class="thresholdRow">
<td>Threshold</td>
<td id="th1">--</td>
<td id="th2">--</td>
<td id="th3">--</td>
<td id="th4">--</td>
<td>--</td>
</tr>
</thead>
<tbody id="tableBody"></tbody>
</table>
</div>
<br>
<button id="pauseBtn" onclick="togglePause()">⏸ Pause</button>
<button onclick="downloadCSV()">Download CSV</button>
<button onclick="clearTable()">Clear Table</button>
</div>

</div>
</div>

<script>
let data=[[],[],[],[]];
let colors=["red","blue","green","orange"];

let paused=false;
let rowCounter=0;

let gmin=0,gmax=4095;

function setRange(){
gmin=Number(document.getElementById("gmin").value);
gmax=Number(document.getElementById("gmax").value);
}

let minArr=[0,0,0,0];
let maxArr=[4095,4095,4095,4095];

function apply(i){
minArr[i]=Number(document.getElementById("min"+i).value);
maxArr[i]=Number(document.getElementById("max"+i).value);
}

let ctxAll=document.getElementById("gAll").getContext("2d");

let canvases=[];
for(let i=0;i<4;i++){
let div=document.createElement("div");
div.innerHTML=`<h3>S${i+1}</h3>
Min<input id="min${i}" value="0"> Max<input id="max${i}" value="4095">
<button onclick="apply(${i})">Apply</button>
<canvas id="g${i}" width="900" height="200"></canvas>`;
document.getElementById("graphs").appendChild(div);
canvases.push(document.getElementById("g"+i).getContext("2d"));
}

function draw(){
ctxAll.clearRect(0,0,900,250);

for(let j=0;j<4;j++){
ctxAll.beginPath();
ctxAll.strokeStyle=colors[j];
for(let i=0;i<data[j].length;i++){
let x=i*10;
let y=250-((data[j][i]-gmin)/(gmax-gmin))*250;
if(i==0) ctxAll.moveTo(x,y);
else ctxAll.lineTo(x,y);
}
ctxAll.stroke();
}

for(let j=0;j<4;j++){
let ctx=canvases[j];
ctx.clearRect(0,0,900,200);

ctx.beginPath();
ctx.strokeStyle=colors[j];

for(let i=0;i<data[j].length;i++){
let x=i*10;
let y=200-((data[j][i]-minArr[j])/(maxArr[j]-minArr[j]))*200;
if(i==0) ctx.moveTo(x,y);
else ctx.lineTo(x,y);
}
ctx.stroke();
}
}

async function update(){
let r=await fetch('/data');
let t=await r.text();
let v=t.split(",");

let th=await fetch('/threshold');
let tt=await th.text();
let parts=tt.split(",");

for(let i=0;i<4;i++){
let val=Number(v[i]);
let threshold=Number(parts[i]);

document.getElementById("v"+(i+1)).innerText=val;

let card=document.getElementById("c"+(i+1));
card.className=(val>=threshold)?"card blood":"card noblood";

data[i].push(val);
if(data[i].length>80) data[i].shift();
}

draw();

let all=await fetch('/all');
let txt=await all.text();
let p=txt.split("|");

document.getElementById("bloodData").innerText=p[0];
document.getElementById("noBloodData").innerText=p[1];
document.getElementById("thresholdData").innerText=p[2];

let tr=await fetch('/time');
document.getElementById("timer").innerText=await tr.text();

// Update threshold cells in table header row
for(let i=0;i<4;i++){
document.getElementById("th"+(i+1)).innerText=Number(parts[i]).toFixed(0);
}

// Add a new row to the data table only when not paused
if(!paused){
let label=0;
let cells="";
let bits=[1,2,4,8];
for(let i=0;i<4;i++){
let val=Number(v[i]);
let threshold=Number(parts[i]);
if(val>=threshold){
cells+=`<td class="bubble">${val}</td>`;
label+=bits[i];
}else{
cells+=`<td>${val}</td>`;
}
}
rowCounter++;
let count=rowCounter;
let labelCell=`<td class="countCol ${label===0?'countOk':''}">${label}</td>`;
let newRow=document.createElement("tr");
newRow.innerHTML=`<td>${count}</td>`+cells+labelCell;
document.getElementById("tableBody").appendChild(newRow);

// Keep only the latest 200 rows in the DOM (scrolling area handles overflow)
while(document.getElementById("tableBody").children.length>200){
document.getElementById("tableBody").removeChild(document.getElementById("tableBody").firstChild);
}
}
}

function showTab(name){
document.getElementById("tabDash").className=(name==="dash")?"tabContent active":"tabContent";
document.getElementById("tabTable").className=(name==="table")?"tabContent active":"tabContent";
document.getElementById("tabBtnDash").className=(name==="dash")?"active":"";
document.getElementById("tabBtnTable").className=(name==="table")?"active":"";
}

function clearTable(){
document.getElementById("tableBody").innerHTML="";
rowCounter=0;
}

function togglePause(){
paused=!paused;
let btn=document.getElementById("pauseBtn");
btn.innerText=paused?"▶ Resume":"⏸ Pause";
btn.style.background=paused?"#43a047":"";
}

async function cal(m){
await fetch('/cal?mode='+m);
}

function downloadCSV(){
let rows=document.querySelectorAll("#dataTable tr");
let csv=[];
rows.forEach(r=>{
let cols=r.querySelectorAll("th,td");
let row=[];
cols.forEach(c=>row.push(c.innerText));
csv.push(row.join(","));
});
let csvContent="data:text/csv;charset=utf-8,"+csv.join("\n");
let link=document.createElement("a");
link.setAttribute("href",encodeURI(csvContent));
link.setAttribute("download","bubble_data.csv");
document.body.appendChild(link);
link.click();
document.body.removeChild(link);
}

setInterval(update,150);
</script>

</body>
</html>
)rawliteral";

// ================= ROUTES =================
void root(){server.send(200,"text/html",page);}

void sendData(){
String d=String(v[0])+","+String(v[1])+","+String(v[2])+","+String(v[3]);
server.send(200,"text/plain",d);
}

void sendThreshold(){
String t="";
for(int i=0;i<4;i++){
float th=(blood_th[i]+noblood_th[i])/2.0;
t+=String(th);
if(i<3)t+=",";
}
server.send(200,"text/plain",t);
}

void sendAll(){
String data="";
for(int i=0;i<4;i++){data+=String(blood_th[i]);if(i<3)data+=",";}
data+="|";
for(int i=0;i<4;i++){data+=String(noblood_th[i]);if(i<3)data+=",";}
data+="|";
for(int i=0;i<4;i++){float th=(blood_th[i]+noblood_th[i])/2.0;data+=String(th);if(i<3)data+=",";}
server.send(200,"text/plain",data);
}

void sendTime(){
if(!calibrating){server.send(200,"text/plain",""); return;}
int remain=30-(millis()-startTime)/1000;
server.send(200,"text/plain","Time Left: "+String(remain)+"s");
}

void startCal(){
mode=server.arg("mode").toInt();
calibrating=true;
startTime=millis();
count=0;
for(int i=0;i<4;i++) sum[i]=0;
server.send(200,"text/plain","OK");
}

// ================= SETUP =================
void setup(){
Serial.begin(115200);

EEPROM.begin(EEPROM_SIZE);

for(int i=0;i<4;i++){
EEPROM.get(i*4,blood_th[i]);
EEPROM.get(16+i*4,noblood_th[i]);
if(isnan(blood_th[i])) blood_th[i]=500;
if(isnan(noblood_th[i])) noblood_th[i]=100;
}

for(int i=0;i<4;i++) pinMode(ledPins[i],OUTPUT);

WiFi.softAP(ssid,password);
Serial.println(WiFi.softAPIP());

server.on("/",root);
server.on("/data",sendData);
server.on("/threshold",sendThreshold);
server.on("/all",sendAll);
server.on("/cal",startCal);
server.on("/time",sendTime);

server.begin();
}

// ================= LOOP =================
void loop(){
server.handleClient();

if(millis()-lastRead>100){
lastRead=millis();

for(int i=0;i<4;i++){
v[i]=analogRead(sensorPins[i]);
}

// LED logic
for(int i=0;i<4;i++){
float th=(blood_th[i]+noblood_th[i])/2.0;
if(v[i] < th) digitalWrite(ledPins[i],HIGH);
else digitalWrite(ledPins[i],LOW);
}

// calibration
if(calibrating){
for(int i=0;i<4;i++) sum[i]+=v[i];
count++;

if(millis()-startTime>30000){
for(int i=0;i<4;i++){
float avg=sum[i]/(float)count;
if(mode==1){blood_th[i]=avg; EEPROM.put(i*4,avg);}
else{noblood_th[i]=avg; EEPROM.put(16+i*4,avg);}
}
EEPROM.commit();
calibrating=false;
}
}
}
}