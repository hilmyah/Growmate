// =============================================================
//  Growmate — PCB Edition
//  Hardware : Wemos D1 Mini, Soil Sensor (A0),
//             Relay (D2/GPIO4, active HIGH), LCD I2C 16x2 (SDA=D4, SCL=D5)
//  Features : Web Dashboard, Blynk, EEPROM, mDNS, OTA,
//             Light/Dark mode, Custom presets
// =============================================================

#define BLYNK_TEMPLATE_ID   "TMPL6BQ_wZk2b"
#define BLYNK_TEMPLATE_NAME "PKK Grafik"
#define BLYNK_AUTH_TOKEN    "W1yClSQOJchmJ-WBRJeuzRpJgNsrKQSg"
#define BLYNK_PRINT         Serial

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// -------------------------------------------------------------
//  WiFi credentials
// -------------------------------------------------------------
const char* ssid     = "fh_0ac9e8";
const char* password = "wlanf53617";

// -------------------------------------------------------------
//  Hardware pins
// -------------------------------------------------------------
const int soilPin  = A0;
const int relayPin = D2;   // GPIO4, active HIGH
#define   SDA_PIN  D4      // GPIO2
#define   SCL_PIN  D5      // GPIO14

// -------------------------------------------------------------
//  LCD — alamat default 0x27; jika tidak tampil, coba 0x3F
// -------------------------------------------------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define PAGE_INTERVAL     3000   // ms antar halaman LCD
static int           lcdPage      = 0;
static unsigned long lastPageMs   = 0;
static bool          lcdPageDirty = true;

unsigned long lastLcdUpdate = 0;
const long    lcdInterval   = 1000;

// -------------------------------------------------------------
//  EEPROM layout
//  [0..1]  int   threshold
//  [2]     byte  presetCount
//  [3..N]  per preset: 13 byte name + 2 byte int thr = 15 byte/slot
// -------------------------------------------------------------
#define EEPROM_ADDR_THRESHOLD    0
#define EEPROM_ADDR_PRESET_COUNT 2
#define EEPROM_ADDR_PRESETS      3
#define PRESET_SLOT_SIZE         15
#define MAX_CUSTOM_PRESETS       10
#define EEPROM_SIZE              512

// -------------------------------------------------------------
//  Runtime state
// -------------------------------------------------------------
ESP8266WebServer server(80);
BlynkTimer       timer;

int  sensorValue  = 0;
int  threshold    = 700;
int  systemMode   = 2;   // 0=Auto  1=Manual ON  2=Manual OFF
bool isPumpOn     = false;

String kondisi    = "BASAH";
String pumpStatus = "OFF";

unsigned long manualStartMillis  = 0;
const long    manualTimeout      = 60000;
unsigned long lastWateredMillis  = 0;
unsigned long lastBlynkReconnect = 0;

int wateringCount = 0;

int moistureHistory[40];
int historyIndex = 0;
int historyCount = 0;

struct Preset { char name[13]; int thr; };
Preset customPresets[MAX_CUSTOM_PRESETS];
int    customPresetCount = 0;


// =============================================================
//  EEPROM helpers
// =============================================================

void loadFromEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(EEPROM_ADDR_THRESHOLD, threshold);
  if (threshold < 0 || threshold > 1024) threshold = 700;

  byte count = EEPROM.read(EEPROM_ADDR_PRESET_COUNT);
  if (count > MAX_CUSTOM_PRESETS) count = 0;
  customPresetCount = count;

  for (int i = 0; i < customPresetCount; i++) {
    int base = EEPROM_ADDR_PRESETS + i * PRESET_SLOT_SIZE;
    for (int c = 0; c < 13; c++) customPresets[i].name[c] = EEPROM.read(base + c);
    customPresets[i].name[12] = '\0';
    EEPROM.get(base + 13, customPresets[i].thr);
  }
  Serial.println("EEPROM loaded — threshold: " + String(threshold)
    + ", presets: " + String(customPresetCount));
}

void saveThreshold() {
  EEPROM.put(EEPROM_ADDR_THRESHOLD, threshold);
  EEPROM.commit();
}

void savePresetsToEEPROM() {
  EEPROM.write(EEPROM_ADDR_PRESET_COUNT, (byte)customPresetCount);
  for (int i = 0; i < customPresetCount; i++) {
    int base = EEPROM_ADDR_PRESETS + i * PRESET_SLOT_SIZE;
    for (int c = 0; c < 13; c++) EEPROM.write(base + c, customPresets[i].name[c]);
    EEPROM.put(base + 13, customPresets[i].thr);
  }
  EEPROM.commit();
}


// =============================================================
//  Pump control
// =============================================================

void updatePumpState() {
  if (isPumpOn) {
    digitalWrite(relayPin, HIGH);
    pumpStatus = "ON";
    if (Blynk.connected()) Blynk.virtualWrite(V3, 1);
  } else {
    digitalWrite(relayPin, LOW);
    pumpStatus = "OFF";
    if (Blynk.connected()) Blynk.virtualWrite(V3, 0);
    lastWateredMillis = millis();
  }
  lcdPageDirty = true;
}


// =============================================================
//  LCD helpers
// =============================================================

// Tulis string tepat 16 karakter (padding spasi) tanpa lcd.clear()
void lcdPrint16(uint8_t row, const char* str) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", str);
  lcd.setCursor(0, row);
  lcd.print(buf);
}

// Paging LCD — 3 halaman berganti tiap PAGE_INTERVAL ms.
// lcd.clear() hanya dipanggil saat ganti halaman (1x per 3 detik),
// bukan tiap update data, agar bus I2C tidak dibanjiri perintah.
void updateLCD() {
  unsigned long now = millis();

  if (now - lastPageMs >= PAGE_INTERVAL) {
    lastPageMs   = now;
    lcdPage      = (lcdPage + 1) % 3;
    lcdPageDirty = true;
    lcd.clear();
  }

  if (!lcdPageDirty) return;
  lcdPageDirty = false;

  char r0[17], r1[17];
  int  pct = constrain(map(sensorValue, 0, 1024, 100, 0), 0, 100);

  if (lcdPage == 0) {
    snprintf(r0, sizeof(r0), "Tanah:%-10s", kondisi.c_str());
    snprintf(r1, sizeof(r1), "ADC:%-4d  %3d%%", sensorValue, pct);
  } else if (lcdPage == 1) {
    const char* mStr = (systemMode == 0) ? "AUTO"
                     : (systemMode == 1) ? "MAN ON"
                     :                     "MAN OFF";
    snprintf(r0, sizeof(r0), "Mode:%-11s", mStr);
    snprintf(r1, sizeof(r1), "Pompa:%-10s", pumpStatus.c_str());
  } else {
    snprintf(r0, sizeof(r0), "Batas:%-10d", threshold);
    snprintf(r1, sizeof(r1), "Disiram:%4d kali", wateringCount);
  }

  lcdPrint16(0, r0);
  lcdPrint16(1, r1);
}

// =============================================================
//  HTML Dashboard
// =============================================================

const char MAIN_page[] PROGMEM =
"<!DOCTYPE html>"
"<html lang='id' data-theme='dark'>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Growmate</title>"
"<script src='https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js'></script>"
"<link href='https://fonts.googleapis.com/css2?family=DM+Sans:wght@400;500;600&family=DM+Mono:wght@400;500&display=swap' rel='stylesheet'>"
"<style>"
"[data-theme=dark]{--bg:#0a0a0a;--s1:#111111;--s2:#181818;--bd:#252525;--bd2:#2f2f2f;--g:#4a7c59;--gh:#6aab7e;--gd:#2d4f38;--am:#b8972a;--rd:#a03030;--tx:#e8e8e8;--tm:#999999;--tl:#666666;--thumb-border:#0a0a0a;}"
"[data-theme=light]{--bg:#f5f5f3;--s1:#ffffff;--s2:#f0f0ee;--bd:#e0e0dc;--bd2:#d0d0cc;--g:#3d6b4a;--gh:#3d6b4a;--gd:#c8dece;--am:#8a6a10;--rd:#8b2020;--tx:#1a1a1a;--tm:#555555;--tl:#888888;--thumb-border:#f5f5f3;}"
"*{box-sizing:border-box;margin:0;padding:0;transition:background .2s,color .2s,border-color .2s;}"
"body{background:var(--bg);font-family:'DM Sans',sans-serif;color:var(--tx);padding:28px 20px;min-height:100vh;}"
".hdr{border-bottom:1px solid var(--bd);padding-bottom:18px;margin-bottom:24px;display:flex;align-items:center;justify-content:space-between;}"
".hdr-left h1{font-size:18px;font-weight:600;letter-spacing:-.2px;}"
".hdr-left p{font-size:11px;color:var(--tl);margin-top:2px;letter-spacing:1.5px;text-transform:uppercase;}"
".hdr-right{display:flex;align-items:center;gap:10px;}"
".badge{font-family:'DM Mono',monospace;font-size:11px;color:var(--gh);background:var(--gd);padding:4px 10px;border-radius:4px;letter-spacing:.5px;}"
".theme-btn{background:transparent;border:1px solid var(--bd2);border-radius:5px;color:var(--tl);font-family:'DM Mono',monospace;font-size:11px;padding:4px 10px;cursor:pointer;transition:border-color .15s,color .15s;letter-spacing:.3px;}"
".theme-btn:hover{border-color:var(--g);color:var(--gh);}"
".grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px;max-width:1000px;margin:0 auto;}"
".full{grid-column:1/-1;}.span2{grid-column:span 2;}"
".card{background:var(--s1);border:1px solid var(--bd);border-radius:8px;padding:16px;}"
".lbl{font-size:10px;text-transform:uppercase;letter-spacing:1.8px;color:var(--tl);margin-bottom:14px;}"
".big{font-size:32px;font-weight:600;letter-spacing:-1px;line-height:1;margin-bottom:6px;}"
".mono{font-family:'DM Mono',monospace;font-size:11px;color:var(--tl);}"
".cg{color:var(--gh);}.ca{color:var(--am);}.cr{color:var(--rd);}.cm{color:var(--tm);}"
".gw{margin-top:14px;}"
".gr{display:flex;justify-content:space-between;font-family:'DM Mono',monospace;font-size:10px;color:var(--tl);margin-bottom:5px;}"
".gt{height:6px;background:var(--s2);border-radius:2px;overflow:hidden;border:1px solid var(--bd);}"
".gf{height:100%;border-radius:2px;transition:width .5s,background .3s;}"
".gz{display:grid;grid-template-columns:1fr 1fr 1fr;gap:4px;margin-top:6px;}"
".gz div{font-size:9px;text-align:center;padding:3px 0;border-radius:2px;letter-spacing:.5px;text-transform:uppercase;}"
".zw{background:rgba(74,124,89,.12);color:var(--gh);border:1px solid rgba(74,124,89,.3);}"
".zn{background:rgba(184,151,42,.12);color:var(--am);border:1px solid rgba(184,151,42,.25);}"
".zd{background:rgba(160,48,48,.12);color:var(--rd);border:1px solid rgba(160,48,48,.25);}"
".led{width:10px;height:10px;border-radius:50%;background:var(--gd);display:inline-block;margin-right:8px;vertical-align:middle;border:1px solid var(--bd2);}"
".led.on{background:var(--gh);border-color:var(--g);}"
".ptxt{font-size:28px;font-weight:600;letter-spacing:-.5px;}"
".cnum{font-size:44px;font-weight:600;letter-spacing:-2px;line-height:1;margin-bottom:4px;}"
".dots{display:flex;gap:4px;flex-wrap:wrap;margin-top:10px;}"
".dot{width:6px;height:6px;border-radius:50%;}"
".br{display:flex;gap:6px;margin-bottom:12px;}"
".cbtn{flex:1;padding:8px 0;border-radius:5px;border:1px solid var(--bd2);background:transparent;color:var(--tl);font-family:'DM Sans',sans-serif;font-size:12px;font-weight:500;cursor:pointer;transition:background .15s,border-color .15s,color .15s;}"
".cbtn:hover{background:rgba(128,128,128,.06);}"
".aa{background:rgba(52,130,200,.1)!important;border-color:#3482c8!important;color:#3482c8!important;}"
".ao{background:rgba(74,124,89,.12)!important;border-color:var(--g)!important;color:var(--gh)!important;}"
".af{background:rgba(160,48,48,.1)!important;border-color:var(--rd)!important;color:var(--rd)!important;}"
".sh{display:flex;justify-content:space-between;align-items:center;font-size:10px;color:var(--tl);letter-spacing:1px;text-transform:uppercase;margin-bottom:6px;}"
".sh span{font-family:'DM Mono',monospace;color:var(--gh);font-size:11px;letter-spacing:0;text-transform:none;}"
"input[type=range]{-webkit-appearance:none;width:100%;height:4px;background:var(--bd2);border-radius:2px;outline:none;cursor:pointer;}"
"input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:14px;height:14px;border-radius:50%;background:var(--gh);cursor:pointer;border:2px solid var(--thumb-border);}"
".ptg{display:grid;grid-template-columns:repeat(auto-fill,minmax(110px,1fr));gap:6px;margin-top:10px;}"
".pti{border:1px solid var(--bd);border-radius:5px;padding:8px;cursor:pointer;transition:border-color .15s;position:relative;background:var(--s1);}"
".pti:hover{border-color:var(--g);}"
".pti.sel{border-color:var(--g);background:var(--gd);}"
".ptn{font-size:11px;font-weight:600;color:var(--tx);margin-bottom:2px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;}"
".ptv{font-family:'DM Mono',monospace;font-size:10px;color:var(--tl);}"
".ptb{height:2px;background:var(--bd);border-radius:1px;margin-top:5px;overflow:hidden;}"
".ptbf{height:100%;background:var(--g);border-radius:1px;}"
".ptd{position:absolute;top:5px;right:6px;font-size:11px;color:var(--tl);cursor:pointer;background:none;border:none;font-family:'DM Sans',sans-serif;padding:0;line-height:1;display:none;}"
".pti:hover .ptd{display:block;}"
".ptd:hover{color:var(--rd);}"
".addcard{border:1px dashed var(--bd2);border-radius:5px;padding:8px;cursor:pointer;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:4px;min-height:58px;transition:border-color .15s,background .15s;}"
".addcard:hover{border-color:var(--g);background:rgba(74,124,89,.04);}"
".addcard span{font-size:10px;color:var(--tl);letter-spacing:.5px;}"
".addicon{font-size:18px;color:var(--tl);line-height:1;font-weight:300;}"
".afw{display:none;margin-top:12px;background:var(--s2);border:1px solid var(--bd2);border-radius:6px;padding:14px;}"
".afw.open{display:block;}"
".frow{display:grid;grid-template-columns:1fr 1fr;gap:10px;}"
".field{display:flex;flex-direction:column;gap:5px;}"
".field label{font-size:10px;text-transform:uppercase;letter-spacing:1.2px;color:var(--tl);display:flex;justify-content:space-between;align-items:center;}"
".field label span{font-family:'DM Mono',monospace;color:var(--gh);text-transform:none;letter-spacing:0;}"
".field input[type=text]{background:var(--s1);border:1px solid var(--bd2);border-radius:4px;color:var(--tx);font-family:'DM Sans',sans-serif;font-size:13px;padding:7px 10px;outline:none;width:100%;transition:border-color .15s;}"
".field input[type=text]:focus{border-color:var(--g);}"
".field input[type=text]::placeholder{color:var(--tl);}"
".fact{display:flex;gap:6px;margin-top:12px;justify-content:flex-end;align-items:center;}"
".bsave{background:var(--gd);border:1px solid var(--g);color:var(--gh);font-family:'DM Sans',sans-serif;font-size:12px;font-weight:600;padding:7px 16px;border-radius:4px;cursor:pointer;}"
".bsave:hover{opacity:.85;}"
".bcancel{background:transparent;border:1px solid var(--bd2);color:var(--tl);font-family:'DM Sans',sans-serif;font-size:12px;padding:7px 16px;border-radius:4px;cursor:pointer;}"
".bcancel:hover{border-color:var(--tm);color:var(--tm);}"
".ferr{font-size:10px;color:var(--rd);margin-top:6px;font-family:'DM Mono',monospace;display:none;}"
".ferr.show{display:block;}"
".fhint{font-size:10px;color:var(--tl);font-family:'DM Mono',monospace;}"
".chart-wrap{margin-top:12px;position:relative;height:90px;}"
".xax{display:flex;justify-content:space-between;margin-top:4px;}"
".xax span{font-family:'DM Mono',monospace;font-size:9px;color:var(--tl);}"
".adj-btn{padding:0 8px;height:22px;font-size:14px;line-height:1;border-radius:4px;border:1px solid var(--bd2);background:var(--s2);color:var(--tx);cursor:pointer;font-weight:bold;}"
".adj-btn:hover{border-color:var(--g);}"
"</style></head><body>"
"<div class='hdr'>"
"<div class='hdr-left'><h1>Growmate</h1><p>Irrigation monitoring system</p></div>"
"<div class='hdr-right'>"
"<button class='theme-btn' id='themeBtn' onclick='toggleTheme()'>Light</button>"
"<div class='badge'>LIVE</div>"
"</div></div>"
"<div class='grid'>"
"<div class='card span2'>"
"<div class='lbl'>Soil Moisture</div>"
"<div class='big' id='kondisiTxt'>--</div>"
"<div class='mono'>ADC <span id='adcTxt' style='color:var(--tx)'>--</span> / 1024 &nbsp;&middot;&nbsp; <span id='pctTxt'>--%</span></div>"
"<div class='gw'><div class='gr'><span>0</span><span>512</span><span>1024</span></div>"
"<div class='gt'><div class='gf' id='gaugeFill' style='width:0%;background:var(--gh)'></div></div>"
"<div class='gz'><div class='zw'>Basah</div><div class='zn'>Normal</div><div class='zd'>Kering</div></div>"
"</div></div>"
"<div class='card'>"
"<div class='lbl'>Pump</div>"
"<div style='margin-bottom:8px'><span class='led' id='pumpLed'></span><span class='ptxt' id='pumpTxt'>--</span></div>"
"<div class='mono'>Mode: <span id='modeTxt' style='color:var(--tm)'>--</span></div>"
"<div class='mono' style='margin-top:4px'>Last: <span id='lastTxt' style='color:var(--tm)'>--</span></div>"
"</div>"
"<div class='card'>"
"<div class='lbl'>Waterings Today</div>"
"<div class='cnum' id='countTxt'>0</div>"
"<div class='mono'>times</div>"
"<div class='dots' id='dotsWrap'></div>"
"</div>"
"<div class='card span2'>"
"<div class='lbl'>System Control</div>"
"<div class='br'>"
"<button id='btnAuto' class='cbtn' onclick=\"cmd('/auto')\">Auto</button>"
"<button id='btnOn'   class='cbtn' onclick=\"cmd('/on')\">ON</button>"
"<button id='btnOff'  class='cbtn' onclick=\"cmd('/off')\">OFF</button>"
"</div>"
"<div class='sh'><span style='font-family:\"DM Sans\",sans-serif;font-size:10px;color:var(--tl);letter-spacing:1px'>Threshold</span>"
"<div style='display:flex;align-items:center;gap:8px'>"
"<button class='adj-btn' onclick='adj(-5)'>-</button>"
"<span id='thrVal'>--</span>"
"<button class='adj-btn' onclick='adj(5)'>+</button>"
"</div></div>"
"<input type='range' id='thrSlider' min='200' max='1024' value='700' step='1' oninput='onSlider(this.value)'>"
"</div>"
"<div class='card full'>"
"<div class='lbl'>Plant Presets</div>"
"<div class='ptg' id='ptGrid'></div>"
"<div class='afw' id='addForm'>"
"<div class='frow'>"
"<div class='field'><label>Plant Name</label><input type='text' id='inName' placeholder='e.g. Selada' maxlength='12'></div>"
"<div class='field'><label>Threshold <span id='inThrVal'>600</span></label>"
"<input type='range' min='200' max='1024' value='600' step='1' id='inThr' oninput=\"document.getElementById('inThrVal').innerText=this.value\"></div>"
"</div>"
"<div class='ferr' id='formErr'>Name cannot be empty.</div>"
"<div class='fact'>"
"<span class='fhint'>Low = wetter &nbsp;&middot;&nbsp; High = drier</span>"
"<button class='bcancel' onclick='closeForm()'>Cancel</button>"
"<button class='bsave' onclick='savePreset()'>Save Preset</button>"
"</div></div></div>"
"<div class='card full'>"
"<div class='lbl'>Moisture History</div>"
"<div class='chart-wrap'><canvas id='soilChart'></canvas></div>"
"<div class='xax'><span id='xL'>--</span><span id='xM'>--</span><span id='xR'>now</span></div>"
"</div></div>"
"<script>"
"function toggleTheme(){const html=document.documentElement;const isDark=html.getAttribute('data-theme')==='dark';const next=isDark?'light':'dark';html.setAttribute('data-theme',next);document.getElementById('themeBtn').innerText=isDark?'Dark':'Light';localStorage.setItem('gm-theme',next);updateChartTheme();}"
"(function(){const s=localStorage.getItem('gm-theme');if(s&&s==='light'){document.documentElement.setAttribute('data-theme','light');document.getElementById('themeBtn').innerText='Dark';}})();"
"const BUILTIN=[{n:'Cabai',t:650},{n:'Tomat',t:600},{n:'Kangkung',t:450},{n:'Bayam',t:500},{n:'Padi',t:350},{n:'Jagung',t:700},{n:'Bawang',t:750},{n:'Kentang',t:680},{n:'Kaktus',t:900},{n:'Stroberi',t:580}];"
"let custom=[];let selIdx=null;"
"function renderPresets(){const g=document.getElementById('ptGrid');g.innerHTML='';const all=[...BUILTIN.map(p=>({...p,custom:false})),...custom.map(p=>({...p,custom:true}))];all.forEach((p,i)=>{const d=document.createElement('div');d.className='pti'+(selIdx===i?' sel':'');d.onclick=()=>selectPreset(i,p.t);const del=p.custom?'<button class=\"ptd\" onclick=\"event.stopPropagation();delPreset('+i+')\">x</button>':'';d.innerHTML=del+'<div class=\"ptn\">'+p.n+'</div><div class=\"ptv\">thr: '+p.t+'</div><div class=\"ptb\"><div class=\"ptbf\" style=\"width:'+Math.round(p.t/1024*100)+'%\"></div></div>';g.appendChild(d);});const add=document.createElement('div');add.className='addcard';add.onclick=openForm;add.innerHTML='<div class=\"addicon\">+</div><span>Add Preset</span>';g.appendChild(add);}"
"function selectPreset(i,t){selIdx=i;document.getElementById('thrSlider').value=t;document.getElementById('thrVal').innerText=t;fetch('/api/threshold?val='+t);renderPresets();}"
"function delPreset(i){const bl=BUILTIN.length;if(i<bl)return;custom.splice(i-bl,1);if(selIdx===i)selIdx=null;else if(selIdx>i)selIdx--;saveCustom();renderPresets();}"
"function openForm(){document.getElementById('addForm').classList.add('open');document.getElementById('inName').focus();document.getElementById('formErr').classList.remove('show');}"
"function closeForm(){document.getElementById('addForm').classList.remove('open');document.getElementById('inName').value='';document.getElementById('inThr').value=600;document.getElementById('inThrVal').innerText=600;}"
"function savePreset(){const n=document.getElementById('inName').value.trim();const t=parseInt(document.getElementById('inThr').value);if(!n){document.getElementById('formErr').classList.add('show');return;}custom.push({n,t});closeForm();renderPresets();saveCustom();}"
"function saveCustom(){fetch('/api/presets',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(custom)});}"
"function onSlider(v){document.getElementById('thrVal').innerText=v;selIdx=null;renderPresets();fetch('/api/threshold?val='+v);}"
"function adj(d){let s=document.getElementById('thrSlider');let v=parseInt(s.value)+d;v=Math.max(200,Math.min(1024,v));s.value=v;onSlider(v);}"
"function cmd(url){fetch(url).then(()=>poll());}"
"function fmtTime(s){if(s===0)return'Just now';if(s<60)return s+'s ago';return Math.floor(s/60)+'m '+(s%60)+'s ago';}"
"const ctx=document.getElementById('soilChart').getContext('2d');"
"Chart.defaults.font.family='DM Mono';"
"const chartData={labels:[],datasets:[{data:[],borderColor:'#4a7c59',backgroundColor:'rgba(74,124,89,.08)',fill:true,tension:.4,borderWidth:1.5,pointRadius:0}]};"
"const chartOpts={responsive:true,maintainAspectRatio:false,animation:false,plugins:{legend:{display:false}},scales:{y:{suggestedMin:0,suggestedMax:1024,grid:{color:'#1e1e1e'},border:{display:false},ticks:{font:{family:'DM Mono',size:9},color:'#666'}},x:{display:false}}};"
"const chart=new Chart(ctx,{type:'line',data:chartData,options:chartOpts});"
"function updateChartTheme(){const light=document.documentElement.getAttribute('data-theme')==='light';chart.options.scales.y.grid.color=light?'#e8e8e4':'#1e1e1e';chart.options.scales.y.ticks.color=light?'#999':'#666';chartData.datasets[0].borderColor=light?'#3d6b4a':'#4a7c59';chartData.datasets[0].backgroundColor=light?'rgba(61,107,74,.08)':'rgba(74,124,89,.08)';chart.update();}"
"function renderDots(n){const w=document.getElementById('dotsWrap');w.innerHTML='';for(let i=0;i<10;i++){const d=document.createElement('div');d.className='dot';d.style.background=i<n?'var(--gh)':'var(--bd2)';w.appendChild(d);}}"
"function poll(){fetch('/api/data').then(r=>r.json()).then(d=>{const adc=d.adc,thr=d.threshold;const pct=Math.max(0,Math.min(100,Math.round((1-adc/1024)*100)));const el=document.getElementById('kondisiTxt');el.innerText=d.kondisi;el.className='big '+(d.kondisi==='BASAH'?'cg':d.kondisi==='NORMAL'?'ca':'cr');document.getElementById('adcTxt').innerText=adc;document.getElementById('pctTxt').innerText=pct+'%';const gf=document.getElementById('gaugeFill');gf.style.width=pct+'%';gf.style.background=adc<thr-50?'var(--gh)':adc<thr+50?'var(--am)':'var(--rd)';const on=d.pump==='ON';document.getElementById('pumpLed').className='led'+(on?' on':'');const pt=document.getElementById('pumpTxt');pt.innerText=d.pump;pt.className='ptxt '+(on?'cg':'cm');document.getElementById('modeTxt').innerText=d.mode;document.getElementById('lastTxt').innerText=fmtTime(d.lastWatered);document.getElementById('thrVal').innerText=thr;document.getElementById('thrSlider').value=thr;document.getElementById('countTxt').innerText=d.count;renderDots(Math.min(d.count,10));document.getElementById('btnAuto').className='cbtn'+(d.mode==='AUTO'?' aa':'');document.getElementById('btnOn').className='cbtn'+(d.mode==='MANUAL ON'?' ao':'');document.getElementById('btnOff').className='cbtn'+(d.mode==='MANUAL OFF'?' af':'');const now=new Date().toLocaleTimeString([],{hour:'2-digit',minute:'2-digit',second:'2-digit'});if(chartData.labels.length>=40){chartData.labels.shift();chartData.datasets[0].data.shift();}chartData.labels.push(now);chartData.datasets[0].data.push(adc);chart.update();const len=chartData.labels.length;if(len>1)document.getElementById('xL').innerText=chartData.labels[0];if(len>2)document.getElementById('xM').innerText=chartData.labels[Math.floor(len/2)];}).catch(()=>{});}"
"fetch('/api/presets').then(r=>r.json()).then(d=>{custom=d;renderPresets();}).catch(()=>renderPresets());"
"setInterval(poll,2000);poll();"
"</script></body></html>";

// =============================================================
//  Web server handlers
// =============================================================

void handleRoot() { server.send(200, "text/html", MAIN_page); }

void handleApi() {
  sensorValue = analogRead(soilPin);
  moistureHistory[historyIndex] = sensorValue;
  historyIndex = (historyIndex + 1) % 40;
  if (historyCount < 40) historyCount++;

  if      (sensorValue < threshold - 50) kondisi = "BASAH";
  else if (sensorValue < threshold + 50) kondisi = "NORMAL";
  else                                    kondisi = "KERING";

  String modeStr = (systemMode == 0) ? "AUTO" : (systemMode == 1) ? "MANUAL ON" : "MANUAL OFF";
  unsigned long elapsed = (lastWateredMillis == 0) ? 0 : (millis() - lastWateredMillis) / 1000;

  String json = "{";
  json += "\"adc\":"         + String(sensorValue) + ",";
  json += "\"kondisi\":\""   + kondisi              + "\",";
  json += "\"pump\":\""      + pumpStatus           + "\",";
  json += "\"mode\":\""      + modeStr              + "\",";
  json += "\"threshold\":"   + String(threshold)    + ",";
  json += "\"lastWatered\":" + String(elapsed)      + ",";
  json += "\"count\":"       + String(wateringCount);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSetThreshold() {
  if (server.hasArg("val")) {
    threshold = server.arg("val").toInt();
    saveThreshold();
    lcdPageDirty = true;
    Serial.println("Threshold updated: " + String(threshold));
  }
  server.send(200, "text/plain", "OK");
}

void handleGetPresets() {
  String json = "[";
  for (int i = 0; i < customPresetCount; i++) {
    if (i > 0) json += ",";
    json += "{\"n\":\"" + String(customPresets[i].name) + "\",\"t\":" + String(customPresets[i].thr) + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handlePostPresets() {
  if (!server.hasArg("plain")) { server.send(400, "text/plain", "No body"); return; }
  String body = server.arg("plain");
  customPresetCount = 0;
  int pos = 0;
  while (pos < (int)body.length() && customPresetCount < MAX_CUSTOM_PRESETS) {
    int ns = body.indexOf("\"n\":\"", pos); if (ns < 0) break; ns += 5;
    int ne = body.indexOf("\"", ns);        if (ne < 0) break;
    int ts = body.indexOf("\"t\":", ne);    if (ts < 0) break; ts += 4;
    int te = ts;
    while (te < (int)body.length() && (isDigit(body[te]) || body[te] == '-')) te++;
    String name = body.substring(ns, ne);
    int    thr  = body.substring(ts, te).toInt();
    if (name.length() > 0 && thr >= 0 && thr <= 1024) {
      name = name.substring(0, min((int)name.length(), 12));
      memset(customPresets[customPresetCount].name, 0, 13);
      name.toCharArray(customPresets[customPresetCount].name, 13);
      customPresets[customPresetCount].thr = thr;
      customPresetCount++;
    }
    pos = te;
  }
  savePresetsToEEPROM();
  server.send(200, "text/plain", "OK");
}

void handleGetHistory() {
  String json = "[";
  int count = min(historyCount, 5);
  int start = (historyIndex - count + 40) % 40;
  for (int i = 0; i < count; i++) {
    if (i > 0) json += ",";
    json += String(moistureHistory[(start + i) % 40]);
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleWebAuto() { systemMode = 0; lcdPageDirty = true; server.send(200, "text/plain", "OK"); }

void handleWebOn() {
  systemMode = 1; isPumpOn = true; manualStartMillis = millis();
  updatePumpState(); server.send(200, "text/plain", "OK");
}

void handleWebOff() {
  systemMode = 2; isPumpOn = false;
  updatePumpState(); server.send(200, "text/plain", "OK");
}


// =============================================================
//  Blynk
// =============================================================

BLYNK_WRITE(V3) {
  int val = param.asInt();
  if (val == 1) { systemMode = 1; isPumpOn = true; manualStartMillis = millis(); }
  else          { systemMode = 2; isPumpOn = false; }
  updatePumpState();
}

void sendToBlynk() {
  if (!Blynk.connected()) return;
  int raw     = analogRead(soilPin);
  int percent = constrain(map(raw, 0, 1024, 100, 0), 0, 100);
  Blynk.virtualWrite(V0, raw);
  Blynk.virtualWrite(V2, percent);
  String m = (systemMode == 0) ? "Auto" : (systemMode == 1) ? "Manual ON" : "Manual OFF";
  Blynk.virtualWrite(V1, m);
}


// =============================================================
//  Setup
// =============================================================

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // I2C & LCD init
  // Urutan Wire.begin → lcd.init → lcd.backlight wajib diikuti.
  // Jika LCD masih menampilkan karakter acak: pastikan VCC ke 5V (bukan 3.3V),
  // atau coba alamat 0x3F, atau jalankan i2c_scanner.ino untuk verifikasi.
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(50);
  lcd.init();
  delay(10);
  lcd.backlight();
  delay(10);

  lcd.clear();
  lcdPrint16(0, "   GROWMATE");
  lcdPrint16(1, " SMKN 1 CIMAHI");
  delay(2000);
  lcd.clear();

  loadFromEEPROM();
  memset(moistureHistory, 0, sizeof(moistureHistory));

  isPumpOn = false;
  updatePumpState();

  // WiFi — animasi titik di baris 1 selama connecting
  lcdPrint16(0, "Connecting WiFi");
  lcdPrint16(1, "");
  WiFi.begin(ssid, password);

  int dotCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    char dots[17] = "";
    for (int i = 0; i < dotCount % 8; i++) dots[i] = '.';
    lcdPrint16(1, dots);
    dotCount++;
  }
  Serial.println("\nWiFi connected — " + WiFi.localIP().toString());

  lcd.clear();
  lcdPrint16(0, "WiFi Terhubung!");
  lcdPrint16(1, WiFi.localIP().toString().c_str());
  delay(2000);
  lcd.clear();

  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect(3000);

  if (MDNS.begin("growmate")) Serial.println("mDNS started — http://growmate.local");

  ArduinoOTA.setHostname("Wemos-Growmate");
  ArduinoOTA.onStart([]() {
    Serial.println("OTA: starting...");
    lcd.clear();
    lcdPrint16(0, "OTA Updating...");
    lcdPrint16(1, "Jangan dimatikan");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA: done.");
    lcd.clear();
    lcdPrint16(0, "OTA Selesai!");
    lcdPrint16(1, "Restarting...");
  });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
    Serial.printf("OTA: %u%%\r", p / (t / 100));
  });
  ArduinoOTA.onError([](ota_error_t e) {
    Serial.printf("OTA error [%u]\n", e);
  });
  ArduinoOTA.begin();

  server.on("/",              handleRoot);
  server.on("/api/data",      handleApi);
  server.on("/api/threshold", handleSetThreshold);
  server.on("/api/presets",   HTTP_GET,  handleGetPresets);
  server.on("/api/presets",   HTTP_POST, handlePostPresets);
  server.on("/api/history",   handleGetHistory);
  server.on("/on",            handleWebOn);
  server.on("/off",           handleWebOff);
  server.on("/auto",          handleWebAuto);
  server.begin();

  timer.setInterval(1000L, sendToBlynk);
  Serial.println("System ready — http://" + WiFi.localIP().toString());

  lastPageMs   = millis();
  lcdPageDirty = true;
  updateLCD();
}


// =============================================================
//  Loop
// =============================================================

void loop() {
  ArduinoOTA.handle();
  MDNS.update();

  if (WiFi.status() == WL_CONNECTED) {
    if (Blynk.connected()) {
      Blynk.run();
    } else if (millis() - lastBlynkReconnect > 10000) {
      lastBlynkReconnect = millis();
      Serial.println("Blynk disconnected — retrying...");
      Blynk.connect(3000);
    }
  }

  server.handleClient();
  timer.run();

  // Manual timeout — kembali ke Auto setelah 60 detik
  if (systemMode == 1 && millis() - manualStartMillis >= manualTimeout) {
    systemMode = 0; isPumpOn = false;
    updatePumpState();
    Serial.println("Manual timeout — switching to Auto");
  }

  // Auto mode — hysteresis ±20 untuk menghindari flutter
  if (systemMode == 0) {
    int raw = analogRead(soilPin);
    if      (raw > threshold + 20 && !isPumpOn) { isPumpOn = true; wateringCount++; updatePumpState(); }
    else if (raw < threshold - 20 &&  isPumpOn) { isPumpOn = false; updatePumpState(); }
  }

  // Update sensor & LCD tiap 1 detik
  if (millis() - lastLcdUpdate >= lcdInterval) {
    lastLcdUpdate = millis();
    sensorValue   = analogRead(soilPin);
    if      (sensorValue < threshold - 50) kondisi = "BASAH";
    else if (sensorValue < threshold + 50) kondisi = "NORMAL";
    else                                    kondisi = "KERING";
    lcdPageDirty = true;
    updateLCD();
  }
}