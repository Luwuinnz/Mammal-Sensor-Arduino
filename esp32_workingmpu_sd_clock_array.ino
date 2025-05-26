//Adruino Module libraries
#include "Wire.h"
#include <MPU6050_light.h>
#include <Bonezegei_DS3231.h>
#include <SD.h>
#include <SPI.h>

//ESP#@ Webserver libraries
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

//GPS Libraries
#include <TinyGPS++.h>
#include <HardwareSerial.h>

//wifi esp32 integration
const char* ssid = "ESP32_DataLogger";
const char* password = "esp32log";

//Webserver login and pw
// const char* ssid = "NETGEAR77";
// const char* password = "livelyviolet795";
// const char* ssid = "PVNET Guest";
// const char* password = "Promenade";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Sensor and RTC setup
MPU6050 mpu(Wire);
Bonezegei_DS3231 rtc(0x68);

// SD card config
const int chipSelect = 5; // SD card CS pin
File myFile;

// I2C Pins for ESP32
#define I2C_SDA 21
#define I2C_SCL 22

#define LED_PIN 2
bool enableLogging = true;
bool loggingToggleRequested = false;


// Create TinyGPS++ object
TinyGPSPlus gps;

// Create a HardwareSerial port (Serial2 for ESP32)
HardwareSerial gpsSerial(2); // use UART2


unsigned long lastGPSUpdate = 0;
bool gpsAvailable = false;
const unsigned long gpsTimeout = 10000; // e.g., 10 seconds

char gpsLat[15] = "";
char gpsLng[15] = "";
char sensorData[12][10];


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booting...");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);


  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD init failed!");
    while (true);
  }
  Serial.println("SD init done.");

  // Init MPU6050
  byte status = mpu.begin();
  Serial.print(F("MPU6050 status: "));
  Serial.println(status);
  while (status != 0) { delay(500); } // Retry until MPU connects

  Serial.println(F("Calculating offsets, do not move MPU6050"));
  delay(1000);
  mpu.calcOffsets(true, true);
  Serial.println("\nMPU6050 ready!");

  gpsSerial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
  Serial.println("\nGPS Tracker Started");

  // Init RTC
  if (!rtc.begin()) {
    Serial.println("RTC not found.");
  }


  // IPAddress local_IP(192, 168, 1, 184);
  // IPAddress gateway(192, 168, 1, 1);
  // IPAddress subnet(255, 255, 255, 0);
  // WiFi.config(local_IP, gateway, subnet);


  const char* requiredFiles[] = {"/index.html", "/style.css", "/script.js", "/chart.js"};
  for (int i = 0; i < 4; i++) {
    if (!SD.exists(requiredFiles[i])) {
      Serial.print("Missing file: "); Serial.println(requiredFiles[i]);
    }
  }
  // ---------- Access Point Configuration ----------
  IPAddress local_IP(192, 168, 12, 3);
  IPAddress gateway(192, 168, 12, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  Serial.println("Access Point started.");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Start Web Server and WebSocket
  setupWebServer();


  // Optional: Write header
  write_csv_header_if_needed();
}

void loop() {
  server.handleClient();
  webSocket.loop();
  static unsigned long lastLog = 0;
  if (millis() - lastLog >= 1000) {
    char timeBuffer[64];

    // Fill timestamp
    clock_time(timeBuffer, sizeof(timeBuffer));
    Serial.println(timeBuffer);

    // Fill MPU data
    mpu_dataAG(sensorData);

    // Print to serial
    Serial.println("Temp C|Acc X      Y      Z |Gyro X    Y      Z  |AcAng X     Y |Angle X     Y     Z");
    for (int i = 0; i < 12; i++) {
      Serial.print(sensorData[i]);
      Serial.print(i < 11 ? "," : "\n");
    }

    gps_data(); //print out gps data

    // Save to SD card
    
    if (enableLogging) {
    if (write_sd_array(timeBuffer, sensorData)) {
      Serial.println("\nData written to SD card.");
    } else {
      Serial.println("Error writing to SD card.");
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("\nData not logged: SD card writing is paused.");
  }

  // SAFELY toggle after logging completes
  if (loggingToggleRequested) {
    enableLogging = !enableLogging;
    loggingToggleRequested = false;
    Serial.println(enableLogging ? "Logging ENABLED" : "Logging DISABLED");
  }


    sendLiveDataToClient(timeBuffer,sensorData);


    Serial.println("\n==============\n");
    lastLog = millis();
  }
}

void mpu_dataAG(char data[][10]) {
  mpu.update();

  dtostrf(mpu.getTemp(), 6, 2, data[0]);
  dtostrf(mpu.getAccX(), 6, 2, data[1]);
  dtostrf(mpu.getAccY(), 6, 2, data[2]);
  dtostrf(mpu.getAccZ(), 6, 2, data[3]);
  dtostrf(mpu.getGyroX(), 6, 2, data[4]);
  dtostrf(mpu.getGyroY(), 6, 2, data[5]);
  dtostrf(mpu.getGyroZ(), 6, 2, data[6]);
  dtostrf(mpu.getAccAngleX(), 6, 2, data[7]);
  dtostrf(mpu.getAccAngleY(), 6, 2, data[8]);
  dtostrf(mpu.getAngleX(), 6, 2, data[9]);
  dtostrf(mpu.getAngleY(), 6, 2, data[10]);
  dtostrf(mpu.getAngleZ(), 6, 2, data[11]);
}

void clock_time(char* buffer, size_t len) {
  if (rtc.getTime()) {
    snprintf(buffer, len, "Time %02d:%02d:%02d Date %02d-%02d-%d",
             rtc.getHour(), rtc.getMinute(), rtc.getSeconds(),
             rtc.getMonth(), rtc.getDate(), rtc.getYear());
  } 
  else {
    strncpy(buffer, "RTC Error", len);
  }
}

bool write_sd_array(const char* timeStr, char data[][10]) {
  if (!enableLogging){
    digitalWrite(LED_PIN, LOW);
    return false;
    }

  digitalWrite(LED_PIN, HIGH); // LED ON while writing
  myFile = SD.open("/test.txt", FILE_APPEND);

  if (myFile) {
    myFile.print(timeStr);
    myFile.print(",");
    for (int i = 0; i < 12; i++) {
      myFile.print(data[i]);
      myFile.print(i < 11 ? "," : "\n");
    }
    myFile.close();
    //digitalWrite(LED_PIN, LOW); // LED OFF after writing
    return true;
  } 
  else {
    digitalWrite(LED_PIN, LOW); // LED OFF on failure
    return false;
  }
}



void write_csv_header_if_needed() {
  if (!SD.exists("/test.txt")) {
    myFile = SD.open("/test.txt", FILE_WRITE);
    if (myFile) {
      myFile.println("Time,Temp,AccX,AccY,AccZ,GyroX,GyroY,GyroZ,AccAngleX,AccAngleY,AngleX,AngleY,AngleZ");
      myFile.close();
    } else {
      Serial.println("Failed to create header.");
    }
  } else {
    Serial.println("Log file already exists, not overwriting header.");
  }
}

void serveIndex() {
  File file = SD.open("/index.html");
  if (file) {
    server.streamFile(file, "text/html");
    file.close();
  } else {
    Serial.println("Failed to open /index.html");
    server.send(404, "text/plain", "index.html not found");
  }
}


void serveStyle() {
  File file = SD.open("/style.css");
  if (file) {
    server.streamFile(file, "text/css");
    file.close();
  } else {
    server.send(404, "text/plain", "style.css not found");
  }
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.println("WebSocket client connected");
  } else if (type == WStype_DISCONNECTED) {
    Serial.println("WebSocket client disconnected");
  }
}



void setupWebServer() {
  server.on("/", serveIndex);
  server.on("/style.css", serveStyle);
  server.on("/script.js", serveScript);
  server.on("/chart.js", serveChartJS);

  server.on("/toggleLogging", []() {
    loggingToggleRequested = true;
    server.send(200, "text/plain", enableLogging ? "Turning OFF..." : "Turning ON...");
  });

  server.on("/loggingStatus", []() {
    server.send(200, "text/plain", enableLogging ? "on" : "off");
  });

  server.begin();
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
}

void serveChartJS() {
  File file = SD.open("/chart.js");
  if (file) {
    server.streamFile(file, "application/javascript");
    file.close();
  } else {
    server.send(404, "text/plain", "chart.js not found");
  }
}


void sendLiveDataToClient(const char* timeStr, char data[][10]) {
  char liveData[256];  // Ensure this is large enough; increase if you add more data
  int pos = 0;

  // Start with the timestamp
  pos += snprintf(liveData + pos, sizeof(liveData) - pos, "%s,", timeStr);

  // Add sensor data
  for (int i = 0; i < 12; i++) {
    pos += snprintf(liveData + pos, sizeof(liveData) - pos, "%s", data[i]);
    if (i < 11) {
      pos += snprintf(liveData + pos, sizeof(liveData) - pos, ",");
    }
  }

  // Add separator and GPS
  pos += snprintf(liveData + pos, sizeof(liveData) - pos, ";\n%s,%s", gpsLat, gpsLng);

  // Check WiFi client and send via WebSocket
  if (WiFi.softAPgetStationNum() > 0) {
    webSocket.broadcastTXT(liveData);
  } else {
    Serial.println("WiFi disconnected. WebSocket message not sent.");
  }
}



void serveScript() {
  File file = SD.open("/script.js");
  if (file) {
    server.streamFile(file, "application/javascript");
    file.close();
  } else {
    server.send(404, "text/plain", "script.js not found");
  }
}

bool gps_data() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isValid()) {
    dtostrf(gps.location.lat(), 10, 6, gpsLat);
    dtostrf(gps.location.lng(), 10, 6, gpsLng);
    lastGPSUpdate = millis();
    gpsAvailable = true;
    Serial.print("\nGPS:");
    Serial.print(gpsLat);
    Serial.print(", ");
    Serial.print(gpsLng);
    return true;
  } 
  else {
    // check for timeout
    if (millis() - lastGPSUpdate > gpsTimeout) {
      gpsAvailable = false;
      strcpy(gpsLat, "NO_FIX");
      strcpy(gpsLng, "NO_FIX");
      Serial.println("GPS signal lost.");
    }
    return false;
  }
}

