#include "Wire.h"
#include <MPU6050_light.h>
#include <Bonezegei_DS3231.h>
#include <SD.h>
#include <SPI.h>

// Sensor and RTC setup
MPU6050 mpu(Wire);
Bonezegei_DS3231 rtc(0x68);

// SD card config
const int chipSelect = 5; // SD card CS pin
File myFile;

// I2C Pins for ESP32
#define I2C_SDA 21
#define I2C_SCL 22

char sensorData[12][10];

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booting...");

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
  Serial.println("MPU6050 ready!");

  // Init RTC
  rtc.begin();

  // Optional: Write header
  write_csv_header_if_needed();
}

void loop() {
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

    // Save to SD card
    if (write_sd_array(timeBuffer, sensorData)) {
      Serial.println("Data written to SD card.");
    } else {
      Serial.println("Error writing to SD card.");
    }

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
  } else {
    strncpy(buffer, "RTC Error", len);
  }
}

bool write_sd_array(const char* timeStr, char data[][10]) {
  myFile = SD.open("/test.txt", FILE_APPEND);

  if (myFile) {
    myFile.print(timeStr);
    myFile.print(",");
    for (int i = 0; i < 12; i++) {
      myFile.print(data[i]);
      myFile.print(i < 11 ? "," : "\n");
    }
    Serial.print("File size: ");
    Serial.println(SD.open("/test.txt").size());
    myFile.close();

  
    return true;
  } 
  else {
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
