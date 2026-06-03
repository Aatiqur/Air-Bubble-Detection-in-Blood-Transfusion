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
</div>

<script>
let data=[[],[],[],[]];
let colors=["red","blue","green","orange"];

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
card.className=(val>threshold)?"card blood":"card noblood";

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
}

async function cal(m){
await fetch('/cal?mode='+m);
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