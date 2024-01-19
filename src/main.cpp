/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp8266-nodemcu-websocket-server-sensor/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

/* Adapted to use a different IMU by Joey Reback at Colorado State University
 * Using code from randomNerdTutorials and Adafruit's LSM9DS1 Tutorial
 * 
 * Purpose:    Demonstrate the functionality of the Adafruit LSM9DS1 IMU
 * Parts used: Adafruit Huzzah ESP8266, LSM9DS1, Adafruit LiPo Battery and LiPo Charger
 * IDE used:   VSCode with PlatformIO plugin
 * TODO:       Threejs integration.
*/


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

//pin definitions
#define LSM9DS1_SCK A5
#define LSM9DS1_MISO 12
#define LSM9DS1_MOSI A4
#define LSM9DS1_XGCS 6
#define LSM9DS1_MCS 5

//Network Credentials
const char* ssid     = "TP-Link_AF39";
const char* password = "65105474";

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
double heading;

Adafruit_LSM9DS1 lsm = Adafruit_LSM9DS1();

// Init IMU
void setupSensor(){
  // 1.) Set the accelerometer range
  lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_4G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_8G);
  //lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_16G);
  
  // 2.) Set the magnetometer sensitivity
  lsm.setupMag(lsm.LSM9DS1_MAGGAIN_4GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_8GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_12GAUSS);
  //lsm.setupMag(lsm.LSM9DS1_MAGGAIN_16GAUSS);

  // 3.) Setup the gyroscope
  lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);
  //lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_500DPS);
  //lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_2000DPS);
}

// Get Sensor Readings and return JSON object
String getSensorReadings(){
  lsm.read();  /* ask it to read in the data */ 
  /* Get a new sensor event */ 
  sensors_event_t a, m, g, temp;

  lsm.getEvent(&a, &m, &g, &temp); 
  readings["accelx"] = String(a.acceleration.x);
  readings["accely"] =  String(a.acceleration.y);
  readings["accelz"] = String(a.acceleration.z);

  readings["magx"] = String(m.magnetic.x);
  readings["magy"] =  String(m.magnetic.y);
  readings["magz"] = String(m.magnetic.z);
  heading = atan2(m.magnetic.y, m.magnetic.x) * RAD_TO_DEG;
  
  readings["magd"] = String(heading);

  readings["gyrox"] = String(g.gyro.x);
  readings["gyroy"] =  String(g.gyro.y);
  readings["gyroz"] = String(g.gyro.z); 

  if (g.gyro.x > 0.5 || g.gyro.x < -0.5) rotationXabs += g.gyro.x; 
  if (g.gyro.y > 0.5 || g.gyro.y < -0.5) rotationYabs += g.gyro.y; 
  if (g.gyro.z > 0.5 || g.gyro.z < -0.5) rotationZabs += g.gyro.z;

  readings["absX"] = String(rotationXabs);
  readings["absY"] = String(rotationYabs);
  readings["absZ"] = String(rotationZabs);
  

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
   Serial.println("LittleFS mounted successfully");
  }
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    //data[len] = 0;
    //String message = (char*)data;
    // Check if the message is "getReadings"
    //if (strcmp((char*)data, "getReadings") == 0) {
      //if it is, send current sensor readings
      String sensorReadings = getSensorReadings();
      Serial.print(sensorReadings);
      notifyClients(sensorReadings);
    //}
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

void initIMU(){
  Serial.println("BEGIN IMU SETUP");
  if (!lsm.begin())
  { //haha infinite loop if wired incorrectly
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
  Serial.println("PASSED WIFI TEST");
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
    //Serial.print(sensorReadings);
    notifyClients(sensorReadings);

  lastTime = millis();

  }

  ws.cleanupClients();
}
