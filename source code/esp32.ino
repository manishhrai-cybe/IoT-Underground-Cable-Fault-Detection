#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// ---- CONFIGURE ----
const char* ssid        = "iPhone";
const char* password    = "8999363672";
const char* alertNumber = "+918999363672";
// -------------------

WebServer server(80);

// UART2 → Arduino
HardwareSerial ArduinoPort(2); // RX=32 TX=33

// UART1 → GSM
HardwareSerial GsmPort(1);     // RX=26 TX=27

// SoftwareSerial → GPS (RX only on GPIO16, TX dummy on GPIO17)
SoftwareSerial GpsPort(16, 17); // RX=16, TX=17 (TX unused)
TinyGPSPlus gps;

String rDist = "NF", yDist = "NF", bDist = "NF";
int rRaw = 0, yRaw = 0, bRaw = 0;

String gpsLat  = "N/A";
String gpsLng  = "N/A";

String lastSMS = "None";

bool rAlerted = false;
bool yAlerted = false;
bool bAlerted = false;

unsigned long lastGpsPrint = 0;

// ─────────────────────────────────────────────
// GSM INIT
// ─────────────────────────────────────────────
void gsmInit() {
  delay(1000);
  GsmPort.println("AT");
  delay(500);
  GsmPort.println("ATE0");
  delay(500);
  GsmPort.println("AT+CMGF=1");
  delay(500);
  Serial.println("[GSM] Ready");
}

// ─────────────────────────────────────────────
// SEND SMS
// ─────────────────────────────────────────────
void sendSMS(String msg) {
  Serial.println("[GSM] Sending SMS...");
  GsmPort.println("AT+CMGS=\"" + String(alertNumber) + "\"");
  delay(1000);
  GsmPort.print(msg);
  delay(500);
  GsmPort.write(26);
  delay(3000);
  lastSMS = msg;
  Serial.println("[GSM] SMS SENT");
}

// ─────────────────────────────────────────────
// UPDATE GPS
// ─────────────────────────────────────────────
void updateGPS() {
  while (GpsPort.available()) {
    gps.encode(GpsPort.read());
  }
  if (gps.location.isUpdated() && gps.location.isValid()) {
    gpsLat = String(gps.location.lat(), 6);
    gpsLng = String(gps.location.lng(), 6);
  }
}

// ─────────────────────────────────────────────
// CHECK FAULTS
// ─────────────────────────────────────────────
void checkAndAlert() {
  String mapsLink = "N/A";
  if (gpsLat != "N/A" && gpsLng != "N/A") {
    mapsLink = "maps.google.com/?q=" + gpsLat + "," + gpsLng;
  }

  if (rDist != "NF" && !rAlerted) {
    sendSMS("FAULT DETECTED\nPhase: R\nDist: " + rDist + " km\nLat: " + gpsLat + "\nLng: " + gpsLng + "\nMap: " + mapsLink);
    rAlerted = true;
  }
  if (rDist == "NF") rAlerted = false;

  if (yDist != "NF" && !yAlerted) {
    sendSMS("FAULT DETECTED\nPhase: Y\nDist: " + yDist + " km\nLat: " + gpsLat + "\nLng: " + gpsLng + "\nMap: " + mapsLink);
    yAlerted = true;
  }
  if (yDist == "NF") yAlerted = false;

  if (bDist != "NF" && !bAlerted) {
    sendSMS("FAULT DETECTED\nPhase: B\nDist: " + bDist + " km\nLat: " + gpsLat + "\nLng: " + gpsLng + "\nMap: " + mapsLink);
    bAlerted = true;
  }
  if (bDist == "NF") bAlerted = false;
}

// ─────────────────────────────────────────────
// READ DATA FROM ARDUINO
// ─────────────────────────────────────────────
void readArduino() {
  while (ArduinoPort.available()) {
    String line = ArduinoPort.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    Serial.println("[RX] " + line);

    int c1 = line.indexOf(',');
    int c2 = line.lastIndexOf(',');
    if (c1 <= 0 || c2 <= c1) continue;

    String ph   = line.substring(0, c1);
    String dist = line.substring(c1 + 1, c2);
    int raw     = line.substring(c2 + 1).toInt();

    if (ph == "R") { rDist = dist; rRaw = raw; Serial.println("[R] " + rDist); }
    if (ph == "Y") { yDist = dist; yRaw = raw; Serial.println("[Y] " + yDist); }
    if (ph == "B") { bDist = dist; bRaw = raw; Serial.println("[B] " + bDist); }
  }
}

// ─────────────────────────────────────────────
// WEB PAGE
// ─────────────────────────────────────────────
void handleRoot() {
  String mapsLink = "https://maps.google.com/?q=" + gpsLat + "," + gpsLng;
  String rClass = (rDist == "NF") ? "ok" : "fault";
  String yClass = (yDist == "NF") ? "ok" : "fault";
  String bClass = (bDist == "NF") ? "ok" : "fault";

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "<title>Cable Fault Monitor</title>";
  html += "<style>";
  html += "body{font-family:Arial;background:#0a0e1a;color:white;text-align:center;padding:30px;}";
  html += "h1{color:#00d2ff;}";
  html += ".card{display:inline-block;background:#1a1f2e;border-radius:12px;padding:25px 40px;margin:15px;min-width:180px;}";
  html += ".phase{font-size:14px;letter-spacing:3px;color:#888;}";
  html += ".value{font-size:48px;font-weight:bold;margin:10px 0;}";
  html += ".ok{color:#00e87a;}";
  html += ".fault{color:#ff2d55;}";
  html += ".info-card{display:inline-block;background:#1a1f2e;border-radius:12px;padding:20px 30px;margin:15px;min-width:220px;text-align:left;}";
  html += ".info-title{font-size:12px;letter-spacing:3px;color:#888;margin-bottom:12px;text-align:center;}";
  html += ".info-row{display:flex;justify-content:space-between;gap:20px;margin:6px 0;font-size:13px;}";
  html += ".info-label{color:#666;}";
  html += ".info-val{color:#00d2ff;font-weight:bold;}";
  html += ".map-btn{display:block;margin-top:14px;padding:8px 0;background:rgba(0,210,255,0.1);border:1px solid rgba(0,210,255,0.3);border-radius:8px;color:#00d2ff;font-size:12px;font-weight:bold;text-align:center;text-decoration:none;letter-spacing:2px;}";
  html += ".sms-card{background:#1a1f2e;border-radius:12px;padding:16px 30px;margin:15px auto;max-width:500px;text-align:left;}";
  html += ".sms-title{font-size:12px;letter-spacing:3px;color:#888;margin-bottom:8px;}";
  html += ".sms-body{font-size:12px;color:#aaa;white-space:pre-wrap;word-break:break-word;}";
  html += "</style></head><body>";
  html += "<h1>IoT ENABLED UNDERGROUND CABLE FAULT DETECTION</h1>";
  html += "<div class='card'><div class='phase'>R PHASE</div><div class='value " + rClass + "'>" + rDist + "</div></div>";
  html += "<div class='card'><div class='phase'>Y PHASE</div><div class='value " + yClass + "'>" + yDist + "</div></div>";
  html += "<div class='card'><div class='phase'>B PHASE</div><div class='value " + bClass + "'>" + bDist + "</div></div>";
  html += "<br><div class='info-card'>";
  html += "<div class='info-title'>GPS LOCATION</div>";
  if (gpsLat != "N/A" && gpsLng != "N/A") {
    html += "<div class='info-row'><span class='info-label'>Latitude</span><span class='info-val'>" + gpsLat + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Longitude</span><span class='info-val'>" + gpsLng + "</span></div>";
    html += "<a class='map-btn' href='" + mapsLink + "' target='_blank'>Open Google Maps</a>";
  } else {
    html += "<div style='color:#666;font-size:12px;text-align:center;padding:10px 0;'>Waiting for GPS...</div>";
  }
  html += "</div>";
  html += "<div class='sms-card'>";
  html += "<div class='sms-title'>LAST SMS SENT</div>";
  html += "<div class='sms-body'>" + lastSMS + "</div>";
  html += "</div>";
  html += "<p style='color:#666;'>Auto Refresh Every 2 Seconds</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// ─────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  ArduinoPort.begin(9600, SERIAL_8N1, 32, 33);
  GsmPort.begin(9600, SERIAL_8N1, 26, 27);
  GpsPort.begin(9600); // SoftwareSerial GPS

  gsmInit();

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! Open: http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
}

// ─────────────────────────────────────────────
// LOOP
// ─────────────────────────────────────────────
void loop() {
  server.handleClient();
  readArduino();
  updateGPS();
  checkAndAlert();

  if (millis() - lastGpsPrint >= 2000) {
    lastGpsPrint = millis();
    Serial.println();
    Serial.println("========== GPS DATA ==========");
    Serial.println("Latitude : " + gpsLat);
    Serial.println("Longitude: " + gpsLng);
    if (gpsLat != "N/A" && gpsLng != "N/A") {
      Serial.println("Maps Link: https://maps.google.com/?q=" + gpsLat + "," + gpsLng);
    }
    Serial.println("================================");
    Serial.println();
  }
}