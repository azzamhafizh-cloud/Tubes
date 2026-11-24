/* AirWatch ESP32 telemetry sender
   Hardware assumed: ESP32 DevKit, DHT22, PMS5003, MQ-135 module.
   Libraries: DHT sensor library (Adafruit), Adafruit Unified Sensor.
   Wiring:
     DHT22 DATA -> GPIO 4 (plus 10k pull-up to 3V3 if bare sensor)
     PMS5003 TX -> ESP32 GPIO 16 (RX2); PMS5003 RX -> GPIO 17 (TX2, optional)
     MQ-135 AO -> GPIO 34 (ensure output <= 3.3V; use divider if needed)
     All GND connected. Power PMS5003 per its module specification.
   IMPORTANT: Set Wi-Fi, API URL, and token below. Never publish this token in a public repo.
*/
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define PMS_RX 16
#define PMS_TX 17
#define MQ135_PIN 34
DHT dht(DHTPIN, DHTTYPE);
HardwareSerial pmsSerial(2);

const char* WIFI_SSID = "NAJEHA";
const char* WIFI_PASSWORD = "nazarlovejunjunn";
const char* API_URL = "https://NAMA-PROJECT.vercel.app/api/telemetry";
const char* DEVICE_TOKEN = "GANTI_DENGAN_DEVICE_API_TOKEN";
const char* DEVICE_ID = "AW-001";
unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 10000;

struct PmsData { uint16_t pm25 = 0; uint16_t pm10 = 0; bool valid = false; };
PmsData readPMS() {
  static uint8_t frame[32];
  while (pmsSerial.available() >= 32) {
    if (pmsSerial.peek() != 0x42) { pmsSerial.read(); continue; }
    if (pmsSerial.read() != 0x42) continue;
    if (pmsSerial.read() != 0x4D) continue;
    frame[0] = 0x42; frame[1] = 0x4D;
    size_t n = pmsSerial.readBytes(frame + 2, 30);
    if (n != 30) return {};
    uint16_t sum = 0; for (int i = 0; i < 30; i++) sum += frame[i];
    uint16_t received = (uint16_t(frame[30]) << 8) | frame[31];
    if (sum != received) continue;
    // Atmospheric environment values: PM1.0 at 10-11, PM2.5 at 12-13, PM10 at 14-15
    PmsData d; d.pm25 = (uint16_t(frame[12]) << 8) | frame[13]; d.pm10 = (uint16_t(frame[14]) << 8) | frame[15]; d.valid = true; return d;
  }
  return {};
}

int estimateAQIFromPM25(float pm) {
  // Approximate US EPA PM2.5 breakpoint interpolation; indicative only, not certified monitoring.
  const float bp[][4] = {{0,12.0,0,50},{12.1,35.4,51,100},{35.5,55.4,101,150},{55.5,150.4,151,200},{150.5,250.4,201,300},{250.5,500.4,301,500}};
  if (pm < 0) pm = 0;
  for (int i=0;i<6;i++) if (pm <= bp[i][1]) return (int)round((bp[i][2] + (bp[i][3]-bp[i][2])*(pm-bp[i][0])/(bp[i][1]-bp[i][0])));
  return 500;
}
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis()-start < 20000) { delay(500); Serial.print("."); }
  Serial.println(WiFi.status() == WL_CONNECTED ? " connected" : " failed");
}
void sendTelemetry(float temp, float humidity, PmsData pm, int gasRaw) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure(); // Prototype only. For production, validate Vercel TLS certificate.
  HTTPClient http;
  if (!http.begin(client, API_URL)) { Serial.println("HTTP begin failed"); return; }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + DEVICE_TOKEN);
  StaticJsonDocument<256> doc;
  doc["device_id"] = DEVICE_ID;
  doc["temperature"] = temp;
  doc["humidity"] = humidity;
  doc["pm25"] = pm.pm25;
  doc["pm10"] = pm.pm10;
  doc["aqi"] = estimateAQIFromPM25(pm.pm25);
  doc["gas_raw"] = gasRaw;
  String body; serializeJson(doc, body);
  int code = http.POST(body);
  Serial.printf("POST %d | %s\n", code, http.getString().c_str());
  http.end();
}
void setup() {
  Serial.begin(115200); dht.begin(); pmsSerial.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);
  analogReadResolution(12); connectWiFi();
}
void loop() {
  connectWiFi();
  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();
    float h = dht.readHumidity(), t = dht.readTemperature();
    PmsData pm = readPMS();
    int gas = analogRead(MQ135_PIN);
    if (isnan(t) || isnan(h)) Serial.println("DHT22 read error");
    else if (!pm.valid) Serial.println("PMS5003 frame not received/invalid");
    else sendTelemetry(t, h, pm, gas);
  }
  delay(20);
}
