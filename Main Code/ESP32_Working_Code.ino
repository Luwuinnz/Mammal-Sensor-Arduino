// ESP32-C3 Dual MPU6050 on One I2C Bus (GPIO 6 = SDA, GPIO 7 = SCL)
#define CORE_DEBUG_LEVEL 0 //make sure to have core debug level in tools set to None
#include <Wire.h>
#include <MPU6050_light.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// #define I2C_SDA 6
// #define I2C_SCL 7
#define LED_PIN 20

#define I2C_SDA 6
#define I2C_SCL 7
//#define LED_PIN 10

// New SD Pin Configuration
#define SD_CS    2  // Chip Select
#define SD_SCK   3  // Clock
#define SD_MOSI  4  // Master Out
#define SD_MISO  5  // Master In

String folderPath = "/dog_files";
int fileCount = 0;
String newFileName;

String logFileName = "";
int logFileNumber = 0;

MPU6050 mpu(Wire);         // MPU at 0x68
MPU6050 mpu1(Wire);        // MPU at 0x69

String dogName = "unknownDog";  // default name

const char* ssid = "ESP32_DataLogger";
const char* password = "esp32log";

WebServer server(80);
WebSocketsServer webSocket(81);

bool enableLogging = true;
bool loggingToggleRequested = false;
char sensorData[24][10];
File myFile;

unsigned long startTime = 0;


void setup() {
  Serial.begin(115200);


  while (!Serial) {
    delay(10);
  }

  delay(5000);
  Serial.println("Booting...");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(I2C_SDA, I2C_SCL);

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed!");
    while (true);
  }
  Serial.println("SD init done.");

  // Create folder if missing
  if (!SD.exists(folderPath)) {
    SD.mkdir(folderPath);
    Serial.println("Created /dog_files folder.");
  }

  Serial.println("Enter dog name (or press Enter to skip):");
  while (Serial.available() == 0) { delay(100); }
  dogName = Serial.readStringUntil('\n');
  dogName.trim();
  if (dogName.length() == 0) dogName = "unknownDog";
  Serial.print("Dog name set to: "); Serial.println(dogName);
 
  generateNewLogFileName();
  write_csv_header_if_needed();



  //byte status = mpu.begin();
  mpu.setAddress(0x68);
  byte status = mpu.begin();  // ✅ correct usage

  Serial.print(F("MPU6050 0x68 status: ")); Serial.println(status);
  while (status != 0) delay(500);
  //Serial.print("m");
  mpu.calcOffsets(true, true);
  //Serial.print("n");
  //byte status1 = mpu1.begin(0x69);
  mpu1.setAddress(0x69);
  byte status1 = mpu1.begin();  // ✅ correct usage
  //Serial.print("o");

  Serial.print(F("MPU6050 0x69 status: ")); Serial.println(status1);
  //Serial.print("p");
  while (status1 != 0) delay(500);
  //Serial.print("q");

  mpu1.calcOffsets(true, true);

  //Serial.print("r");

  //setupWebServer();

  //Serial.print("s");
  write_csv_header_if_needed();

  //Serial.print("t");

  WiFi.softAPConfig(IPAddress(192,168,12,3), IPAddress(192,168,12,1), IPAddress(255,255,255,0));
  //Serial.print("u");
  WiFi.softAP(ssid, password);
  //Serial.print("v");

  Serial.println("Access Point started.");
  //Serial.print("w");
  Serial.println(WiFi.softAPIP());
  //Serial.print("x");
  setupWebServer();


  startTime = millis();  // record moment logging starts

}

void loop() {
  server.handleClient();
  webSocket.loop();
  static unsigned long lastLog = 0;

  if (millis() - lastLog >= 70) {
    char timeBuffer[64];
    clock_time(timeBuffer, sizeof(timeBuffer));
    mpu_dataAG(sensorData);

    Serial.print("MPU A & B data @ "); Serial.println(timeBuffer);
    for (int i = 0; i < 24; i++) {
      Serial.print(sensorData[i]); Serial.print(" ");
    }
    Serial.println();

    if (enableLogging) {
      if (write_sd_array(timeBuffer, sensorData)) Serial.println("Data written to SD");
      else Serial.println("SD write failed");
    } else {
      digitalWrite(LED_PIN, LOW);
      Serial.println("Logging paused");
    }

    if (loggingToggleRequested) {
      enableLogging = !enableLogging;
      loggingToggleRequested = false;
    }

    sendLiveDataToClient(timeBuffer, sensorData);
    lastLog = millis();
  }
}

void mpu_dataAG(char data[][10]) {
  mpu.update();
  mpu1.update();

  dtostrf(mpu.getTemp(), 6, 2, data[0]);
  dtostrf(mpu.getAccY(), 6, 2, data[1]);
  dtostrf(-mpu.getAccX(), 6, 2, data[2]);
  dtostrf(mpu.getAccZ(), 6, 2, data[3]);
  dtostrf(mpu.getGyroY(), 6, 2, data[4]);
  dtostrf(-mpu.getGyroX(), 6, 2, data[5]);
  dtostrf(mpu.getGyroZ(), 6, 2, data[6]);
  dtostrf(mpu.getAccAngleX(), 6, 2, data[7]);
  dtostrf(mpu.getAccAngleY(), 6, 2, data[8]);
  dtostrf(mpu.getAngleX(), 6, 2, data[9]);
  dtostrf(mpu.getAngleY(), 6, 2, data[10]);
  dtostrf(mpu.getAngleZ(), 6, 2, data[11]);

  dtostrf(mpu1.getTemp(), 6, 2, data[12]);
  dtostrf(mpu1.getAccY(), 6, 2, data[13]);
  dtostrf(-mpu1.getAccX(), 6, 2, data[14]);
  dtostrf(mpu1.getAccZ(), 6, 2, data[15]);
  dtostrf(mpu1.getGyroY(), 6, 2, data[16]);
  dtostrf(-mpu1.getGyroX(), 6, 2, data[17]);
  dtostrf(mpu1.getGyroZ(), 6, 2, data[18]);
  dtostrf(mpu1.getAccAngleX(), 6, 2, data[19]);
  dtostrf(mpu1.getAccAngleY(), 6, 2, data[20]);
  dtostrf(mpu1.getAngleX(), 6, 2, data[21]);
  dtostrf(mpu1.getAngleY(), 6, 2, data[22]);
  dtostrf(mpu1.getAngleZ(), 6, 2, data[23]);
}

// void clock_time(char* buffer, size_t len) {
//   unsigned long ms = millis();
//   unsigned long s = ms / 1000, m = s / 60, h = m / 60;
//   snprintf(buffer, len, "Uptime %02lu:%02lu:%02lu", h % 24, m % 60, s % 60);
// }

void clock_time(char* buffer, size_t len) {
  unsigned long ms = millis() - startTime;  // elapsed time since data logging started
  unsigned long s = ms / 1000, m = s / 60, h = m / 60;
  snprintf(buffer, len, "Uptime %02lu:%02lu:%02lu", h % 24, m % 60, s % 60);
}


bool write_sd_array(const char* timeStr, char data[][10]) {
  if (!enableLogging) return false;
  digitalWrite(LED_PIN, HIGH);

  myFile = SD.open(logFileName.c_str(), FILE_APPEND);

  //myFile = SD.open("/test.txt", FILE_APPEND);
  if (myFile) {
    myFile.print(timeStr); myFile.print(",");
    for (int i = 0; i < 24; i++) {
      myFile.print(data[i]);
      myFile.print((i == 11) ? ";" : (i < 23 ? "," : "\n"));
    }
    myFile.close();
    return true;
  } 
  else {
    // Blink visibly on failure
    for (int i = 0; i < 5; i++) {  // Blink 5 times
      digitalWrite(LED_PIN, HIGH);
      delay(150);  // LED ON duration
      digitalWrite(LED_PIN, LOW);
      delay(150);  // LED OFF duration
    }
    return false;
  }
}

void write_csv_header_if_needed() {
  myFile = SD.open(logFileName.c_str(), FILE_WRITE);
  if (myFile) {
    myFile.println("Time,A_Temp,A_AccX,A_AccY,A_AccZ,A_GyroX,A_GyroY,A_GyroZ,A_AccAngX,A_AccAngY,A_AngleX,A_AngleY,A_AngleZ;B_Temp,B_AccX,B_AccY,B_AccZ,B_GyroX,B_GyroY,B_GyroZ,B_AccAngX,B_AccAngY,B_AngleX,B_AngleY,B_AngleZ");
    myFile.close();
    Serial.print("New log file created with header: ");
    Serial.println(logFileName);
  } else {
    Serial.println("Failed to create log file for header.");
  }
}


void sendLiveDataToClient(const char* timeStr, char data[][10]) {
  char liveData[512];
  snprintf(liveData, sizeof(liveData), "%s", timeStr);
  strcat(liveData, ",");
  for (int i = 0; i < 24; i++) {
    strcat(liveData, data[i]);
    if (i < 23) strcat(liveData, ",");
  }
  webSocket.broadcastTXT(liveData);
}

// void setupWebServer() {
//   server.on("/", []() {
//       server.send(200, "text/plain", "Welcome to ESP32 Data Logger!");
//     });

//   server.on("/toggleLogging", []() {
//     loggingToggleRequested = true;
//     server.send(200, "text/plain", enableLogging ? "Turning OFF..." : "Turning ON...");
//   });

//   server.on("/loggingStatus", []() {
//     server.send(200, "text/plain", enableLogging ? "on" : "off");
//   });

//   server.begin();
//   webSocket.begin();
//   webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
//     if (type == WStype_CONNECTED) Serial.println("WebSocket connected");
//     else if (type == WStype_DISCONNECTED) Serial.println("WebSocket disconnected");
//   });
// }
String getContentType(String filename) {
  if (filename.endsWith(".htm") || filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".svg")) return "image/svg+xml";
  else if (filename.endsWith(".json")) return "application/json";
  else return "text/plain";
}


void setupWebServer() {
  server.onNotFound([]() {
    String path = server.uri();

    if (path.endsWith("/")) path += "index.html";
    Serial.print("Request for: "); Serial.println(path);

    // SD.open needs full path
    File file = SD.open("/" + path);
    if (file) {
      server.streamFile(file, getContentType(path));
      file.close();
    } else {
      server.send(404, "text/plain", "404: File Not Found");
    }
  });

  server.begin();

  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_CONNECTED) Serial.println("WebSocket connected");
    else if (type == WStype_DISCONNECTED) Serial.println("WebSocket disconnected");
  });
}

int countExistingLogs() {
  int count = 0;
  File dir = SD.open(folderPath.c_str());
  if (!dir || !dir.isDirectory()) {
    Serial.println("Failed to open /dog_files.");
    return 0;
  }

  File entry = dir.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) count++;
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  return count;
}

void generateNewLogFileName() {
  logFileNumber = countExistingLogs() + 1;
  char buffer[40];
  sprintf(buffer, "/dog_files/%s_%03d.txt", dogName.c_str(), logFileNumber);
  logFileName = String(buffer);
  
  Serial.print("New file will be: ");
  Serial.println(logFileName);
}

