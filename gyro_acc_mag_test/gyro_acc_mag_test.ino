#include <Wire.h>

#define MPU9250_ADDR 0x69  // MPU9250 Address when AD0 = HIGH
#define AK8963_ADDR  0x0C  // Magnetometer Address

// MPU9250 Register Addresses
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H  0x43
#define PWR_MGMT_1   0x6B
#define INT_PIN_CFG  0x37

// AK8963 Register Addresses
#define AK8963_CNTL1  0x0A
#define AK8963_ST1    0x02
#define AK8963_HXL    0x03

void setup() {
    Serial.begin(115200);
    Wire.begin();

    // Wake up MPU9250
    writeByte(MPU9250_ADDR, PWR_MGMT_1, 0x00);
    delay(100);

    // Enable bypass to talk to the magnetometer
    writeByte(MPU9250_ADDR, INT_PIN_CFG, 0x02);
    delay(10);

    // Initialize AK8963 (16-bit continuous measurement mode)
    writeByte(AK8963_ADDR, AK8963_CNTL1, 0x16);
    delay(10);
}

void loop() {
    int16_t accel[3], gyro[3], mag[3];

    // Read accelerometer and gyroscope
    readMPU9250Data(accel, gyro);

    // Read magnetometer
    readMagnetometer(mag);

    // Print Data
    Serial.print("Accel: ");
    Serial.print(accel[0]); Serial.print(" ");
    Serial.print(accel[1]); Serial.print(" ");
    Serial.print(accel[2]); Serial.print(" | ");

    Serial.print("Gyro: ");
    Serial.print(gyro[0]); Serial.print(" ");
    Serial.print(gyro[1]); Serial.print(" ");
    Serial.print(gyro[2]); Serial.print(" | ");

    Serial.print("Mag: ");
    Serial.print(mag[0]); Serial.print(" ");
    Serial.print(mag[1]); Serial.print(" ");
    Serial.print(mag[2]); Serial.println();

    delay(500);
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
    // Check data ready
    Wire.beginTransmission(AK8963_ADDR);
    Wire.write(AK8963_ST1);
    Wire.endTransmission(false);
    Wire.requestFrom(AK8963_ADDR, 1);
    if (!(Wire.read() & 0x01)) return;  // No new data

    // Read magnetometer data
    Wire.beginTransmission(AK8963_ADDR);
    Wire.write(AK8963_HXL);
    Wire.endTransmission(false);
    Wire.requestFrom(AK8963_ADDR, 6);
    
    for (int i = 0; i < 3; i++) {
        mag[i] = (Wire.read() | (Wire.read() << 8));
    }
}

void writeByte(uint8_t address, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}
