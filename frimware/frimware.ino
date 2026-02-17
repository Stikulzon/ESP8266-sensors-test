#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define WIFI_SSID       "Zefir"
#define WIFI_PASS       "1234567890"
#define API_KEY         "1234567890"
#define SERVER_PORT     8085

#define TFT_CS          D8
#define TFT_RST         D4
#define TFT_DC          D3
#define BME_ADDR        0x76

const long DISPLAY_INTERVAL = 1000;
const long PC_TIMEOUT_MS    = 3000;
const int  WIFI_TIMEOUT_MS  = 15000;

#define C_BLACK   ST77XX_BLACK
#define C_WHITE   ST77XX_WHITE
#define C_RED     ST77XX_RED
#define C_GREEN   ST77XX_GREEN
#define C_BLUE    ST77XX_BLUE
#define C_YELLOW  ST77XX_YELLOW
#define C_CYAN    ST77XX_CYAN

ESP8266WebServer server(SERVER_PORT);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_BME280 bme;

struct PCStats {
  float cpuTemp = 0.0;
  float gpuTemp = 0.0;
  int fanSpeed = 0;
  bool isOnline = false;
  unsigned long lastUpdate = 0;
} pcData;

unsigned long prevDisplayMillis = 0;
bool initialDrawComplete = false;
bool previousOnlineState = false;

void setupWiFi();
void initHardware();
void handleRoot();
void handleUpdate();
void drawStaticLayout();
void updateDynamicData();

void setup() {
  Serial.begin(115200);
  
  initHardware();
  setupWiFi();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.collectHeaders("X-API-Key");
  server.begin();
  
  Serial.printf("HTTP server started on port %d\n", SERVER_PORT);
  
  drawStaticLayout(); 
}

void loop() {
  server.handleClient();

  unsigned long currentMillis = millis();

  if (pcData.isOnline && (currentMillis - pcData.lastUpdate > PC_TIMEOUT_MS)) {
    pcData.isOnline = false;
  }

  if (currentMillis - prevDisplayMillis >= DISPLAY_INTERVAL) {
    prevDisplayMillis = currentMillis;
    updateDynamicData();
  }
}

void initHardware() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(C_BLACK);
  tft.setTextWrap(false);
  
  if (!bme.begin(BME_ADDR)) {
    Serial.println(F("Error: BME280 not found!"));
    tft.setTextColor(C_RED);
    tft.setCursor(5, 10);
    tft.println(F("BME280 Error!"));
    delay(2000);
  }
}

void setupWiFi() {
  tft.fillScreen(C_BLACK);
  tft.setCursor(5, 10);
  tft.setTextColor(C_WHITE);
  tft.println(F("Connecting to:"));
  tft.setTextColor(C_GREEN);
  tft.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\nWiFi Connected"));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("\nWiFi Timeout!"));
    tft.setTextColor(C_RED);
    tft.println(F("Connection Failed"));
    delay(2000);
  }
}

void drawStaticLayout() {
  tft.fillScreen(C_BLACK);
  tft.setTextSize(1);
  
  tft.setCursor(5, 5);
  tft.setTextColor(C_WHITE);
  tft.print(F("PC Status: "));

  tft.setTextColor(C_WHITE);
  tft.setCursor(5, 20);  tft.print(F("CPU:"));
  tft.setCursor(75, 20); tft.print(F("GPU:"));
  tft.setCursor(5, 35);  tft.print(F("FAN:"));

  tft.drawLine(0, 52, tft.width(), 52, C_BLUE);

  tft.setCursor(5, 60);
  tft.setTextColor(C_YELLOW); 
  tft.print(F("Room Temp: "));

  tft.setCursor(5, 75);
  tft.setTextColor(C_CYAN); 
  tft.print(F("Humidity:  "));

  tft.setCursor(5, 90);
  tft.setTextColor(C_GREEN); 
  tft.print(F("Pressure:  "));
}

void updateDynamicData() {
  if (pcData.isOnline != previousOnlineState) {
    tft.fillRect(65, 5, 80, 10, C_BLACK);
    previousOnlineState = pcData.isOnline;
  }

  tft.setCursor(65, 5);
  if (pcData.isOnline) {
    tft.setTextColor(C_GREEN, C_BLACK);
    tft.print(F("Online "));
  } else {
    tft.setTextColor(C_RED, C_BLACK);
    tft.print(F("Offline"));
  }

  if (pcData.isOnline) {
    tft.setTextColor(C_WHITE, C_BLACK);
    
    tft.fillRect(35, 20, 35, 8, C_BLACK); 
    tft.setCursor(35, 20); 
    tft.print(pcData.cpuTemp, 0); tft.print(F(" C"));

    tft.fillRect(105, 20, 35, 8, C_BLACK);
    tft.setCursor(105, 20); 
    tft.print(pcData.gpuTemp, 0); tft.print(F(" C"));

    tft.fillRect(35, 35, 45, 8, C_BLACK);
    tft.setCursor(35, 35); 
    tft.print(pcData.fanSpeed); tft.print(F(" RPM"));

  } else {
    tft.setTextColor(C_YELLOW, C_BLACK);
    
    tft.fillRect(0, 18, 160, 30, C_BLACK); 
    
    tft.setCursor(5, 20);
    tft.setTextColor(C_WHITE);
    tft.print(F("Connect to IP:"));
    
    tft.setCursor(5, 35);
    tft.setTextColor(C_YELLOW);
    tft.print(WiFi.localIP());
  }

  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;

  tft.setTextColor(C_WHITE, C_BLACK);

  tft.setCursor(70, 60);
  tft.fillRect(70, 60, 50, 8, C_BLACK);
  tft.print(temp, 1); tft.print(F(" C"));

  tft.setCursor(70, 75);
  tft.fillRect(70, 75, 50, 8, C_BLACK);
  tft.print(hum, 1); tft.print(F(" %"));

  tft.setCursor(70, 90);
  tft.fillRect(70, 90, 60, 8, C_BLACK);
  tft.print(pres, 0); tft.print(F(" hPa"));
}

void handleRoot() {
  String html = F("<!DOCTYPE html><html><head><title>ESP8266 Monitor</title>"
  "<meta name='viewport' content='width=device-width, initial-scale=1'>"
  "<style>body{background-color:#2c3e50; color:#ecf0f1; font-family:Arial,sans-serif; text-align:center; padding-top:50px;}"
  "h1{color:#3498db;} p{font-size:1.2rem;}</style></head>"
  "<body><h1>ESP8266 Sensor Monitor</h1>"
  "<p>Status: <b>Running</b></p>"
  "</body></html>");
  
  server.send(200, "text/html", html);
}

void handleUpdate() {
  if (!server.hasHeader("X-API-Key") || server.header("X-API-Key") != API_KEY) {
    server.send(401, "text/plain", "Unauthorized");
    Serial.println(F("401 Unauthorized: Invalid API Key"));
    return;
  }

  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Bad Request");
    Serial.println(F("400 Bad Request: No payload"));
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "text/plain", "JSON Parsing Failed");
    Serial.print(F("JSON Error: ")); Serial.println(error.c_str());
    return;
  }

  pcData.cpuTemp = doc["cpu_temp"] | 0.0;
  pcData.gpuTemp = doc["gpu_temp"] | 0.0;
  pcData.fanSpeed = doc["fan_speed"] | 0;
  
  pcData.isOnline = true;
  pcData.lastUpdate = millis();

  server.send(200, "text/plain", "OK");
}