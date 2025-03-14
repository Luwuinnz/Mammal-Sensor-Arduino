#include <Wire.h>
#include "MadgwickAHRS.h"

#define MPU9250_ADDR 0x69  // MPU9250 Address when AD0 = HIGH
#define AK8963_ADDR  0x0C  // Magnetometer Address

#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H  0x43
#define PWR_MGMT_1   0x6B
#define INT_PIN_CFG  0x37

#define AK8963_CNTL1  0x0A
#define AK8963_ST1    0x02
#define AK8963_HXL    0x03
#define AK8963_ST2    0x09

#define SMOOTHING_FACTOR 5
float accelBuffer[3][SMOOTHING_FACTOR] = {0};
float gyroBuffer[3][SMOOTHING_FACTOR] = {0};
int bufferIndex = 0;

// Create Madgwick filter instance
Madgwick filter;
float sampleFreq = 100.0f; // Adjust based on loop frequency

void setup() {
    Serial.begin(115200);
    Wire.begin();

    // Wake up MPU9250
    writeByte(MPU9250_ADDR, PWR_MGMT_1, 0x00);
    delay(500);

    // Enable bypass to talk to the magnetometer
    writeByte(MPU9250_ADDR, INT_PIN_CFG, 0x02);
    delay(10);

    // Initialize AK8963 (16-bit continuous measurement mode)
    initMagnetometer();

    // Initialize Madgwick filter
    filter.begin(sampleFreq);
}

void loop() {
    int16_t rawAccel[3], rawGyro[3], rawMag[3];

    // Read raw sensor data
    readMPU9250Data(rawAccel, rawGyro);
    readMagnetometer(rawMag);

    // Convert raw sensor data to proper units
    float ax = rawAccel[0] / 16384.0f;  // Convert to g
    float ay = rawAccel[1] / 16384.0f;
    float az = rawAccel[2] / 16384.0f;

    float gx = rawGyro[0] / 131.0f;  // Convert to deg/s
    float gy = rawGyro[1] / 131.0f;
    float gz = rawGyro[2] / 131.0f;

    float mx = rawMag[0] * 0.6f;  // Convert to µT (magnetometer scale factor)
    float my = rawMag[1] * 0.6f;
    float mz = rawMag[2] * 0.6f;

    // Apply Madgwick filter
    filter.update(gx, gy, gz, ax, ay, az, mx, my, mz);

    // Get Euler angles (degrees)
    float roll = filter.getRoll();
    float pitch = filter.getPitch();
    float yaw = filter.getYaw();

    // Print Data
    Serial.print("[x] Pitch: "); Serial.print(pitch);
    Serial.print(" | [Y] Roll: "); Serial.print(roll);
    Serial.print(" | [Z] Yaw: "); Serial.println(yaw);

    delay(60); // Adjust based on desired output speed
}

void readMPU9250Data(int16_t* accel, int16_t* gyro) {
    Wire.beginTransmission(MPU9250_ADDR);
    Wire.write(ACCEL_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU9250_ADDR, 14);

    for (int i = 0; i < 3; i++) {
        accel[i] = (Wire.read() << 8) | Wire.read();
    }
    
    Wire.read(); Wire.read();  // Skip temperature registers

    for (int i = 0; i < 3; i++) {
        gyro[i] = (Wire.read() << 8) | Wire.read();
    }
}

void readMagnetometer(int16_t* mag) {
    Wire.beginTransmission(AK8963_ADDR);
    Wire.write(AK8963_ST1);
    Wire.endTransmission(false);
    Wire.requestFrom(AK8963_ADDR, 1);
    if (!(Wire.read() & 0x01)) return;  // No new data

    Wire.beginTransmission(AK8963_ADDR);
    Wire.write(AK8963_HXL);
    Wire.endTransmission(false);
    Wire.requestFrom(AK8963_ADDR, 6);

    for (int i = 0; i < 3; i++) {
        uint8_t low = Wire.read();
        uint8_t high = Wire.read();
        mag[i] = (high << 8) | low;
    }

    Wire.beginTransmission(AK8963_ADDR);
    Wire.write(AK8963_ST2);
    Wire.endTransmission(false);
    Wire.requestFrom(AK8963_ADDR, 1);
    if (Wire.read() & 0x08) {
        Serial.println("Magnetometer Overflow!");
    }
}

void initMagnetometer() {
    writeByte(AK8963_ADDR, AK8963_CNTL1, 0x00);  // Power down
    delay(100);
    writeByte(AK8963_ADDR, AK8963_CNTL1, 0x06);  // Set to 16-bit continuous mode
    delay(100);
}

void writeByte(uint8_t address, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}
