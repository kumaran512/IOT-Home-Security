/*
  DHARSSHITH ESP32-CAM - MJPEG WEB STREAM TEST (CONSERVATIVE CAPTURE)
  Target: AI-Thinker ESP32-CAM with OV2640

  Purpose:
    - Test camera capture together with Wi-Fi and MJPEG streaming.
    - Keep the servo disconnected and remove radar/servo code for isolation.
    - Serve a minimal web interface on port 80 and MJPEG on port 81.

  Wi-Fi AP: DHARSSHITH-CAM
  Password: CamSecure123
  Web page: http://192.168.4.1/
  Direct MJPEG: http://192.168.4.1:81/stream
*/

#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "esp_http_server.h"
#include <ESP32Servo.h>
#include <Preferences.h>

// AI-Thinker ESP32-CAM pin mapping
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

static const char *AP_SSID = "DHARSSHITH-CAM";
static const char *AP_PASSWORD = "CamSecure123";

WebServer webServer(80);
httpd_handle_t streamHttpd = nullptr;

// Stage 1: manual servo control only. Auto radar will be added after this passes.
static constexpr int SERVO_GPIO = 13;
static constexpr int SERVO_MIN_US = 500;
static constexpr int SERVO_MAX_US = 2400;
Servo panServo;
Preferences preferences;
int angleIR = 35;
int anglePedal = 90;
int angleFlame = 145;
int currentAngle = 90;

// Stage 2: non-blocking automatic radar sweep.
bool radarRunning = false;
int radarMinAngle = 20;
int radarMaxAngle = 160;
int radarStepDegrees = 2;
unsigned long radarIntervalMs = 80;
unsigned long radarLastStepMs = 0;
int radarDirection = 1;

static int clampAngle(int angle) { return constrain(angle, 0, 180); }
static void moveServoTo(int angle) {
  currentAngle = clampAngle(angle);
  panServo.write(currentAngle);
  Serial.printf("Servo moved to %d degrees\n", currentAngle);
}

static void stopRadar(const char *reason) {
  if (radarRunning) Serial.printf("Radar stopped: %s at %d degrees\n", reason, currentAngle);
  radarRunning = false;
}

static void radarTick() {
  if (!radarRunning) return;
  const unsigned long now = millis();
  if (now - radarLastStepMs < radarIntervalMs) return;
  radarLastStepMs = now;

  int nextAngle = currentAngle + radarDirection * radarStepDegrees;
  if (nextAngle >= radarMaxAngle) {
    nextAngle = radarMaxAngle;
    radarDirection = -1;
  } else if (nextAngle <= radarMinAngle) {
    nextAngle = radarMinAngle;
    radarDirection = 1;
  }
  currentAngle = clampAngle(nextAngle);
  panServo.write(currentAngle);
}

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>DHARSSHITH ESP32-CAM</title><style>
:root{color-scheme:dark;--bg:#10151d;--card:#1b2532;--line:#344356;--accent:#49c6a8;--text:#f3f6fa;--muted:#aab7c7}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:16px/1.45 system-ui,Arial,sans-serif}header{padding:18px 14px;background:#17212d;border-bottom:1px solid var(--line);text-align:center}h1{font-size:1.3rem;margin:0 0 4px}header p{margin:0;color:var(--muted)}main{max-width:900px;margin:auto;padding:14px;display:grid;gap:14px}.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:14px}h2{font-size:1.05rem;margin:0 0 10px}.video{background:#05080b;border-radius:10px;min-height:180px;display:flex;align-items:center;justify-content:center;overflow:hidden}img{display:block;width:100%;height:auto;max-height:70vh;object-fit:contain}.small{font-size:.88rem;color:var(--muted)}button{border:0;border-radius:8px;padding:10px 14px;background:var(--accent);color:#06251e;font-weight:700;cursor:pointer;margin:4px}input[type=number]{width:85px;padding:9px;border-radius:7px;border:1px solid var(--line);background:#0e141c;color:var(--text)}.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin:10px 0}.status{padding:10px;border-radius:8px;background:#0e141c;color:var(--accent);overflow-wrap:anywhere}
</style></head><body><header><h1>DHARSSHITH ESP32-CAM</h1><p>Live camera + manual servo aiming</p></header><main>
<section class="card"><h2>Live camera</h2><div class="video"><img id="stream" alt="Waiting for camera stream…"></div><button onclick="reloadStream()">Reload Stream</button><p class="small">Keep the camera on a stable 5 V supply. Servo movement is added without changing the camera configuration.</p></section>
<section class="card"><h2>Manual servo control</h2><p class="small">Servo signal: GPIO 13. Choose a preset or enter a custom angle.</p>
<div class="row"><label for="customAngle">Custom angle (0–180°)</label><input id="customAngle" type="number" min="0" max="180" value="90"><button onclick="moveTo(Number(document.getElementById('customAngle').value))">Move</button></div>
<div class="row"><label>IR sensor</label><input id="irAngle" type="number" min="0" max="180" value="35"><button onclick="aim('ir')">Aim IR</button></div>
<div class="row"><label>Pedal sensor</label><input id="pedalAngle" type="number" min="0" max="180" value="90"><button onclick="aim('pedal')">Aim Pedal</button></div>
<div class="row"><label>Flame sensor</label><input id="flameAngle" type="number" min="0" max="180" value="145"><button onclick="aim('flame')">Aim Flame</button></div>
<div class="row"><button onclick="saveAngles()">Save angle settings</button><span id="servoStatus" class="status">Ready</span></div></section>
<section class="card"><h2>Automatic radar</h2><p class="small">Non-blocking left-right servo sweep. Manual aiming stops radar automatically.</p>
<div class="row"><label for="radarMin">Minimum angle</label><input id="radarMin" type="number" min="0" max="180" value="20"><label for="radarMax">Maximum angle</label><input id="radarMax" type="number" min="0" max="180" value="160"></div>
<div class="row"><label for="radarStep">Step (degrees)</label><input id="radarStep" type="number" min="1" max="10" value="2"><label for="radarInterval">Interval (ms)</label><input id="radarInterval" type="number" min="20" max="1000" step="10" value="80"></div>
<div class="row"><button onclick="startRadar()">Start Radar</button><button onclick="stopRadar()">Stop / Hold</button><span id="radarStatus" class="status">Radar stopped</span></div></section>
<section class="card"><h2>Connection status</h2><div id="status" class="status">Connecting to stream…</div><p class="small">Web interface: port 80 · MJPEG stream: port 81 · QVGA 320 × 240</p></section></main>
<script>
const img=document.getElementById('stream'),statusBox=document.getElementById('status');let retryTimer;
function reloadStream(){clearTimeout(retryTimer);statusBox.textContent='Connecting to camera stream…';img.src='http://'+location.hostname+':81/stream?ts='+Date.now()}
img.onload=()=>statusBox.textContent='Stream connected. Confirm the image is updating.';img.onerror=()=>{statusBox.textContent='Stream disconnected; retrying…';clearTimeout(retryTimer);retryTimer=setTimeout(reloadStream,2000)};reloadStream();
async function api(path){const r=await fetch(path,{cache:'no-store'});const j=await r.json();if(!r.ok||!j.ok)throw new Error(j.message||'Request failed');return j}
async function moveTo(a){a=Math.max(0,Math.min(180,Math.round(a)));document.getElementById('customAngle').value=a;try{const j=await api('/servo?angle='+a);document.getElementById('servoStatus').textContent='Servo at '+j.angle+'°';document.getElementById('radarStatus').textContent='Radar stopped (manual control)'}catch(e){document.getElementById('servoStatus').textContent='Error: '+e.message}}
async function aim(which){const el=document.getElementById(which+'Angle');await moveTo(Number(el.value))}
async function startRadar(){const q=new URLSearchParams({min:document.getElementById('radarMin').value,max:document.getElementById('radarMax').value,step:document.getElementById('radarStep').value,interval:document.getElementById('radarInterval').value});try{const j=await api('/radar/start?'+q);document.getElementById('radarStatus').textContent='Radar running: '+j.min+'°–'+j.max+'°'}catch(e){document.getElementById('radarStatus').textContent='Error: '+e.message}}
async function stopRadar(){try{const j=await api('/radar/stop');document.getElementById('radarStatus').textContent='Radar stopped at '+j.angle+'°'}catch(e){document.getElementById('radarStatus').textContent='Error: '+e.message}}
async function saveAngles(){try{const q=new URLSearchParams({ir:document.getElementById('irAngle').value,pedal:document.getElementById('pedalAngle').value,flame:document.getElementById('flameAngle').value});await api('/save?'+q);document.getElementById('servoStatus').textContent='Angle settings saved'}catch(e){document.getElementById('servoStatus').textContent='Error: '+e.message}}
async function loadAngles(){try{const j=await api('/angles');for(const k of ['ir','pedal','flame'])document.getElementById(k+'Angle').value=j[k]}catch(e){}}
loadAngles();
</script></body></html>
)HTML";

static bool initCamera() {
  Serial.println("\n========== CAMERA INITIALIZATION ==========");
  Serial.printf("Free heap before init: %u bytes\n", (unsigned int)ESP.getFreeHeap());
  Serial.printf("PSRAM found: %s\n", psramFound() ? "YES" : "NO");
  if (psramFound()) {
    Serial.printf("PSRAM total: %u bytes\n", (unsigned int)ESP.getPsramSize());
    Serial.printf("PSRAM free before init: %u bytes\n", (unsigned int)ESP.getFreePsram());
  }

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  // Conservative diagnostic configuration: one frame buffer, wait for the next
  // frame, and a slower XCLK. This tests whether continuous streaming is
  // failing because of the multi-buffer/latest-frame capture configuration.
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("FAIL: esp_camera_init returned 0x%x\n", (unsigned int)err);
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor == nullptr) {
    Serial.println("FAIL: esp_camera_sensor_get() returned NULL");
    return false;
  }
  sensor->set_framesize(sensor, FRAMESIZE_QVGA);
  sensor->set_quality(sensor, 12);
  sensor->set_brightness(sensor, 0);
  sensor->set_saturation(sensor, 0);
  delay(1000);

  // Verify a frame can be obtained before Wi-Fi starts. This separates
  // initialization failures from failures that begin only during streaming.
  camera_fb_t *warmup = esp_camera_fb_get();
  if (warmup == nullptr) {
    Serial.println("WARNING: warm-up capture before Wi-Fi returned NULL");
  } else {
    Serial.printf("PASS: warm-up capture before Wi-Fi: %u bytes\n",
                  (unsigned int)warmup->len);
    esp_camera_fb_return(warmup);
  }

  Serial.println("PASS: Camera initialized (10 MHz XCLK, 1 frame buffer)");
  Serial.printf("Sensor PID: 0x%04X\n", sensor->id.PID);
  Serial.printf("Frame size status: %d (QVGA expected)\n", sensor->status.framesize);
  Serial.printf("Free heap after init: %u bytes\n", (unsigned int)ESP.getFreeHeap());
  if (psramFound()) Serial.printf("Free PSRAM after init: %u bytes\n", (unsigned int)ESP.getFreePsram());
  Serial.println("===========================================");
  return true;
}

static esp_err_t streamHandler(httpd_req_t *req) {
  static const char *CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
  static const char *BOUNDARY = "\r\n--frame\r\n";
  static const char *PART_HEADER = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

  esp_err_t res = httpd_resp_set_type(req, CONTENT_TYPE);
  if (res != ESP_OK) return res;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");
  httpd_resp_set_hdr(req, "Pragma", "no-cache");

  Serial.println("MJPEG client connected");
  unsigned long frameCount = 0;
  unsigned int consecutiveNulls = 0;
  unsigned long lastStats = millis();

  while (true) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == nullptr) {
      consecutiveNulls++;
      if (consecutiveNulls == 1 || consecutiveNulls % 10 == 0) {
        Serial.printf("MJPEG: camera_fb_get() NULL (%u consecutive)\n", consecutiveNulls);
      }
      if (consecutiveNulls >= 30) {
        Serial.println("MJPEG: stopping stream after 30 consecutive capture failures");
        res = ESP_FAIL;
        break;
      }
      delay(30);
      continue;
    }
    consecutiveNulls = 0;

    if (fb->format != PIXFORMAT_JPEG || fb->len == 0) {
      Serial.printf("MJPEG: unexpected frame format=%d length=%u\n", (int)fb->format, (unsigned int)fb->len);
      esp_camera_fb_return(fb);
      delay(10);
      continue;
    }

    char header[96];
    size_t headerLen = snprintf(header, sizeof(header), PART_HEADER, (unsigned int)fb->len);
    res = httpd_resp_send_chunk(req, BOUNDARY, strlen(BOUNDARY));
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, header, headerLen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, "\r\n", 2);

    // Always return the frame buffer, even if the client disconnected during sending.
    esp_camera_fb_return(fb);
    if (res != ESP_OK) break;

    frameCount++;
    unsigned long now = millis();
    if (now - lastStats >= 5000) {
      Serial.printf("MJPEG: sent %lu frames in last 5 seconds; heap=%u; PSRAM free=%u\n",
                    frameCount, (unsigned int)ESP.getFreeHeap(),
                    psramFound() ? (unsigned int)ESP.getFreePsram() : 0U);
      frameCount = 0;
      lastStats = now;
    }
    delay(1);
  }

  Serial.println("MJPEG client disconnected");
  return res;
}

static void startStreamServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;
  config.ctrl_port = 32769;
  config.max_uri_handlers = 4;
  config.lru_purge_enable = true;
  config.stack_size = 8192;

  httpd_uri_t streamUri = {};
  streamUri.uri = "/stream";
  streamUri.method = HTTP_GET;
  streamUri.handler = streamHandler;
  streamUri.user_ctx = nullptr;

  esp_err_t err = httpd_start(&streamHttpd, &config);
  if (err != ESP_OK) {
    Serial.printf("FAIL: httpd_start on port 81 returned 0x%x\n", (unsigned int)err);
    return;
  }
  err = httpd_register_uri_handler(streamHttpd, &streamUri);
  if (err != ESP_OK) {
    Serial.printf("FAIL: registering /stream returned 0x%x\n", (unsigned int)err);
    httpd_stop(streamHttpd);
    streamHttpd = nullptr;
    return;
  }
  Serial.println("PASS: MJPEG server started on port 81");
  Serial.println("Direct stream: http://192.168.4.1:81/stream");
}

static void handleRoot() {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send_P(200, "text/html", INDEX_HTML);
}

static void handleHealth() {
  String json = "{\"camera\":\"initialized\",\"psram\":";
  json += psramFound() ? "true" : "false";
  json += ",\"freeHeap\":" + String(ESP.getFreeHeap());
  json += ",\"freePsram\":" + String(psramFound() ? ESP.getFreePsram() : 0);
  json += ",\"stream\":\"http://192.168.4.1:81/stream\"}";
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", json);
}

static void handleServo() {
  if (!webServer.hasArg("angle")) { webServer.send(400, "application/json", "{\"ok\":false,\"message\":\"Missing angle\"}"); return; }
  stopRadar("manual servo command");
  moveServoTo(webServer.arg("angle").toInt());
  webServer.send(200, "application/json", String("{\"ok\":true,\"angle\":") + currentAngle + "}");
}

static void handleRadarStart() {
  int minAngle = webServer.hasArg("min") ? webServer.arg("min").toInt() : radarMinAngle;
  int maxAngle = webServer.hasArg("max") ? webServer.arg("max").toInt() : radarMaxAngle;
  int step = webServer.hasArg("step") ? webServer.arg("step").toInt() : radarStepDegrees;
  unsigned long interval = webServer.hasArg("interval") ? (unsigned long)webServer.arg("interval").toInt() : radarIntervalMs;

  minAngle = clampAngle(minAngle);
  maxAngle = clampAngle(maxAngle);
  if (minAngle >= maxAngle) {
    webServer.send(400, "application/json", "{\"ok\":false,\"message\":\"Minimum angle must be less than maximum angle\"}");
    return;
  }
  if (step < 1 || step > 10) {
    webServer.send(400, "application/json", "{\"ok\":false,\"message\":\"Step must be 1 to 10 degrees\"}");
    return;
  }
  if (interval < 20 || interval > 1000) {
    webServer.send(400, "application/json", "{\"ok\":false,\"message\":\"Interval must be 20 to 1000 ms\"}");
    return;
  }

  radarMinAngle = minAngle;
  radarMaxAngle = maxAngle;
  radarStepDegrees = step;
  radarIntervalMs = interval;
  radarDirection = (currentAngle >= radarMaxAngle) ? -1 : 1;
  if (currentAngle < radarMinAngle || currentAngle > radarMaxAngle) {
    currentAngle = radarMinAngle;
    panServo.write(currentAngle);
    radarDirection = 1;
  }
  radarRunning = true;
  radarLastStepMs = millis();
  Serial.printf("Radar started: min=%d max=%d step=%d interval=%lu ms\n", radarMinAngle, radarMaxAngle, radarStepDegrees, radarIntervalMs);
  String json = String("{\"ok\":true,\"running\":true,\"min\":") + radarMinAngle + ",\"max\":" + radarMaxAngle + ",\"angle\":" + currentAngle + "}";
  webServer.send(200, "application/json", json);
}

static void handleRadarStop() {
  stopRadar("web stop/hold command");
  String json = String("{\"ok\":true,\"running\":false,\"angle\":") + currentAngle + "}";
  webServer.send(200, "application/json", json);
}

static void handleAngles() {
  String json = String("{\"ok\":true,\"ir\":") + angleIR + ",\"pedal\":" + anglePedal + ",\"flame\":" + angleFlame + "}";
  webServer.send(200, "application/json", json);
}

static void handleSaveAngles() {
  if (webServer.hasArg("ir")) angleIR = clampAngle(webServer.arg("ir").toInt());
  if (webServer.hasArg("pedal")) anglePedal = clampAngle(webServer.arg("pedal").toInt());
  if (webServer.hasArg("flame")) angleFlame = clampAngle(webServer.arg("flame").toInt());
  preferences.putInt("ir", angleIR);
  preferences.putInt("pedal", anglePedal);
  preferences.putInt("flame", angleFlame);
  webServer.send(200, "application/json", "{\"ok\":true}");
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println();
  Serial.println("===============================================");
  Serial.println("DHARSSHITH ESP32-CAM - STREAM + SERVO + AUTO RADAR STAGE 2");
  Serial.println("Camera configuration kept at the proven working settings");
  Serial.println("===============================================");

  if (!initCamera()) {
    Serial.println("Camera initialization failed; halting.");
    while (true) delay(1000);
  }

  // Initialize servo only after camera warm-up succeeds; leave camera settings unchanged.
  panServo.setPeriodHertz(50);
  panServo.attach(SERVO_GPIO, SERVO_MIN_US, SERVO_MAX_US);
  delay(250);
  preferences.begin("cam-aim", false);
  angleIR = clampAngle(preferences.getInt("ir", angleIR));
  anglePedal = clampAngle(preferences.getInt("pedal", anglePedal));
  angleFlame = clampAngle(preferences.getInt("flame", angleFlame));
  moveServoTo(90);

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("FAIL: Wi-Fi access point could not start");
    while (true) delay(1000);
  }

  Serial.printf("Wi-Fi AP: %s\n", AP_SSID);
  Serial.printf("Password: %s\n", AP_PASSWORD);
  Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());

  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/health", HTTP_GET, handleHealth);
  webServer.on("/servo", HTTP_GET, handleServo);
  webServer.on("/radar/start", HTTP_GET, handleRadarStart);
  webServer.on("/radar/stop", HTTP_GET, handleRadarStop);
  webServer.on("/angles", HTTP_GET, handleAngles);
  webServer.on("/save", HTTP_GET, handleSaveAngles);
  webServer.onNotFound([]() { webServer.send(404, "text/plain", "Not found"); });
  webServer.begin();
  Serial.println("PASS: Web interface started on port 80");

  startStreamServer();
  Serial.println("\nConnect your phone to DHARSSHITH-CAM, then open http://192.168.4.1/");
  Serial.println("Servo manual control and non-blocking auto radar enabled on GPIO 13.");
  Serial.println("For direct stream test open http://192.168.4.1:81/stream");
  Serial.println("===============================================");
}

void loop() {
  webServer.handleClient();
  radarTick();
  delay(1);
}
