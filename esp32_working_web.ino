//Adruino Module libraries
#include "Wire.h"
#include <MPU6050_light.h>
#include <Bonezegei_DS3231.h>
#include <SD.h>
#include <SPI.h>

// TwoWire Wire1 = TwoWire(1);  // Custom I2C bus
MPU6050 mpu1(Wire1);         // Second MPU on Wire1 (I2C1)

//ESP#@ Webserver libraries
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

//wifi esp32 integration
const char* ssid = "ESP32_DataLogger";
const char* password = "esp32log";

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

char sensorData[24][10];


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booting...");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);


  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire1.begin(25, 26); // SDA=25, SCL=26 (choose unused pins on your board)


  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD init failed!");
    while (true);
  }
  Serial.println("SD init done.");

  // Init MPU6050
  byte status = mpu.begin();
  Serial.print(F("MPU6050 A status: "));
  Serial.println(status);
  while (status != 0) { delay(500); } // Retry until MPU connects

  Serial.println(F("Calculating offsets, do not move MPU6050"));
  delay(1000);
  mpu.calcOffsets(true, true);
  Serial.println("\nMPU6050 A ready!");

  byte status1 = mpu1.begin();
  Serial.print(F("\nMPU6050 B status: "));
  Serial.println(status1);
  while (status1 != 0) { delay(500); } // Retry until MPU connects

  Serial.println(F("Calculating offsets, do not move MPU6050"));
  delay(1000);
  mpu1.calcOffsets(true, true);
  Serial.println("\nMPU6050 B ready!");


  // Init RTC
  if (!rtc.begin()) {
    Serial.println("RTC not found.");
  }



  const char* requiredFiles[] = {"/index.html", "/style.css", "/script.js", "/chart.js", "/chartjs-plugin-streaming.js", "/chartjs-adapter-date-fns.js", "/three.min.js", "/GLTFLoader.js", "dog.glb"};
  for (int i = 0; i < 9; i++) {
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
  
  if (millis() - lastLog >= 70) { //pull 10 data in 1 second
    char timeBuffer[64];

    // Fill timestamp
    clock_time(timeBuffer, sizeof(timeBuffer));
    Serial.println(timeBuffer);

    // Fill MPU data
    mpu_dataAG(sensorData);

    // Print to serial
    Serial.println("Sensor|Temp C|Acc X      Y      Z |Gyro X    Y      Z  |AcAng X     Y |Angle X     Y     Z");
    for (int i = 0; i < 24; i++) {
      if (i == 0) Serial.print("MPU A ");
      if (i == 12) Serial.print("\nMPU B ");
      Serial.print(sensorData[i]);
      Serial.print(" ");
    }
    Serial.println();


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
  mpu1.update();

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

  dtostrf(mpu1.getTemp(), 6, 2, data[12]);
  dtostrf(mpu1.getAccX(), 6, 2, data[13]);
  dtostrf(mpu1.getAccY(), 6, 2, data[14]);
  dtostrf(mpu1.getAccZ(), 6, 2, data[15]);
  dtostrf(mpu1.getGyroX(), 6, 2, data[16]);
  dtostrf(mpu1.getGyroY(), 6, 2, data[17]);
  dtostrf(mpu1.getGyroZ(), 6, 2, data[18]);
  dtostrf(mpu1.getAccAngleX(), 6, 2, data[19]);
  dtostrf(mpu1.getAccAngleY(), 6, 2, data[20]);
  dtostrf(mpu1.getAngleX(), 6, 2, data[21]);
  dtostrf(mpu1.getAngleY(), 6, 2, data[22]);
  dtostrf(mpu1.getAngleZ(), 6, 2, data[23]);

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
    for (int i = 0; i < 24; i++) {
      myFile.print(data[i]);
      if (i == 11) {
        myFile.print(";");
      } else if (i < 23) {
        myFile.print(",");
      } else {
        myFile.print("\n");
      }
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
      myFile.println("Time,"
        "A_Temp,A_AccX,A_AccY,A_AccZ,A_GyroX,A_GyroY,A_GyroZ,A_AccAngX,A_AccAngY,A_AngleX,A_AngleY,A_AngleZ;"
        "B_Temp,B_AccX,B_AccY,B_AccZ,B_GyroX,B_GyroY,B_GyroZ,B_AccAngX,B_AccAngY,B_AngleX,B_AngleY,B_AngleZ");
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

void serveGLTFLoader() {
  File file = SD.open("/GLTFLoader.js");
  if (file) {
    server.streamFile(file, "application/javascript");
    file.close();
  } else {
    server.send(404, "text/plain", "GLTFLoader.js not found");
  }
}


void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.println("WebSocket client connected");
  } else if (type == WStype_DISCONNECTED) {
    Serial.println("WebSocket client disconnected");
  }
}

void serveThreeJS() {
  File file = SD.open("/three.min.js");
  if (file) {
    server.streamFile(file, "application/javascript");
    file.close();
  } else {
    server.send(404, "text/plain", "three.min.js not found");
  }
}


void setupWebServer() {
  server.on("/", serveIndex);
  server.on("/style.css", serveStyle);
  server.on("/script.js", serveScript);
  server.on("/chart.js", serveChartJS);
  server.on("/chartjs-plugin-streaming.js", serveChartStreamJS);
  server.on("/chartjs-adapter-date-fns.js", serveChartDateJS);
  server.on("/three.min.js", serveThreeJS);
  server.on("/GLTFLoader.js", serveGLTFLoader);
  server.on("/dog.glb", serveDogModel);


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

void serveChartStreamJS() {
  File file = SD.open("/chartjs-plugin-streaming.js");
  if (file) {
    server.streamFile(file, "application/javascript");
    file.close();
  } else {
    server.send(404, "text/plain", "chartjs-plugin-streaming.js not found");
  }
}
void serveChartDateJS() {
  File file = SD.open("/chartjs-adapter-date-fns.js");
  if (file) {
    server.streamFile(file, "application/javascript");
    file.close();
  } else {
    server.send(404, "text/plain", "chartjs-adapter-date-fns.js not found");
  }
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
  char liveData[512];  // Make sure it's big enough
  int pos = 0;

  // Append timeStr + comma
  for (int i = 0; timeStr[i] != '\0' && pos < sizeof(liveData) - 1; i++) {
    liveData[pos++] = timeStr[i];
  }
  if (pos < sizeof(liveData) - 1) liveData[pos++] = ',';  // add comma

  // Append data array, separated by commas
  for (int i = 0; i < 24 && pos < sizeof(liveData) - 1; i++) {
    // Append each string in data[i]
    for (int j = 0; data[i][j] != '\0' && pos < sizeof(liveData) - 1; j++) {
      liveData[pos++] = data[i][j];
    }

    // Append comma except after last element
    if (i < 23 && pos < sizeof(liveData) - 1) {
      liveData[pos++] = ',';
    }
  }

  // Null terminate
  liveData[pos] = '\0';

  // Broadcast
  webSocket.broadcastTXT(liveData);
}


void serveDogModel() {
  File file = SD.open("/dog.glb");
  if (file) {
    server.streamFile(file, "model/gltf-binary");
    file.close();
  } else {
    server.send(404, "text/plain", "dog.glb not found");
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


