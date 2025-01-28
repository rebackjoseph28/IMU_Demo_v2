#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LSM9DS1.h>
#include <Adafruit_Sensor.h>
#include <Arduino_JSON.h>

// Pin definitions
#define LSM9DS1_SCK A5
#define LSM9DS1_MISO 12
#define LSM9DS1_MOSI A4
#define LSM9DS1_XGCS 6
#define LSM9DS1_MCS 5

// Declination in Boulder, CO.
#define DECLINATION -8.58 

// Access Point Credentials
const char* ssid = "ESP8266_AP";
const char* password = "12345678";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 50;

double rotationXabs = 0;
double rotationYabs = 0;
double rotationZabs = 0;
double heading = 0;
String direction;

Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();
sensors_event_t a, m, g, temp;

// Initialize IMU
void setupSensor() {
  lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);
  lsm.setupMag(lsm.LSM9DS1_MAGGAIN_4GAUSS);
  lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);
}

String headingHandler(double xGauss, double yGauss) {
  if ((xGauss < 0 && yGauss < 0) || (xGauss < 0 && yGauss > 0)) heading = atan2(yGauss, xGauss) - PI;
  else heading = atan2(yGauss, xGauss);

  if (heading > 2 * PI) heading -= (2 * PI);
  if (heading < 0) heading += (2 * PI);

  heading *= 180.0 / PI;
  heading += DECLINATION;

  String output;
  if (heading > 337.25 || heading < 22.5) output = "North, ";
  else if (heading > 292.5 && heading <= 337.25) output = "North West, ";
  else if (heading > 247.5 && heading <= 292.5) output = "West, ";
  else if (heading > 202.5 && heading <= 247.5) output = "South West, ";
  else if (heading > 157.5 && heading <= 202.5) output = "South, ";
  else if (heading > 112.5 && heading <= 157.5) output = "South East, ";
  else if (heading > 67.5 && heading <= 112.5) output = "East, ";
  else if (heading > 0 && heading <= 67.5) output = "North East, ";
  
  else output = "what?";
  return output + String(heading);
}

// Get Sensor Readings and return JSON object
String getSensorReadings() {
  lsm.read();  
  lsm.getEvent(&a, &m, &g, &temp);

  readings["accelx"] = String(a.acceleration.x);
  readings["accely"] = String(a.acceleration.y);
  readings["accelz"] = String(a.acceleration.z);

  readings["magx"] = String(m.magnetic.x + 36.92);
  readings["magy"] = String(m.magnetic.y - 3.07);
  readings["magz"] = String(m.magnetic.z - 13.94);

  readings["gyrox"] = String(g.gyro.x);
  readings["gyroy"] = String(g.gyro.y);
  readings["gyroz"] = String(g.gyro.z); 

  if (g.gyro.x > 0.06 || g.gyro.x < -0.06) rotationXabs += g.gyro.x; 
  if (g.gyro.y > 0.06 || g.gyro.y < -0.06) rotationYabs += g.gyro.y;
  if (g.gyro.z > 0.06 || g.gyro.z < -0.06) rotationZabs += g.gyro.z;

  readings["absX"] = String(rotationXabs);
  readings["absY"] = String(rotationYabs);
  readings["absZ"] = String(rotationZabs);
  
  direction = headingHandler(m.magnetic.x + 36.92, m.magnetic.y - 3.07);
  readings["magd"] = String(direction);

  return JSON.stringify(readings);
}

// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  } else {
    Serial.println("LittleFS mounted successfully");
  }
}

// Initialize Access Point
void initWiFi() {
  WiFi.mode(WIFI_AP);  // Set the ESP8266 to AP mode
  WiFi.softAP(ssid, password);  // Start the AP with credentials
  Serial.println("Access Point Started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
}

void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      String sensorReadings = getSensorReadings();
      notifyClients(sensorReadings);
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void initIMU() {
  Serial.println("BEGIN IMU SETUP");
  if (!lsm.begin()) {
    Serial.println("Unable to initialize the LSM9DS1. Check wiring!");
    while (1);
  }
  Serial.println("Found LSM9DS1 9DOF");
  setupSensor();
}

void setup() {
  Serial.begin(9600);
  Serial.println("IMU Demo V2");
  initWiFi();
  initFS();
  initWebSocket();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  // Start server
  server.begin();

  initIMU();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    String sensorReadings = getSensorReadings();
    notifyClients(sensorReadings);

    lastTime = millis();
  }

  ws.cleanupClients();
}
