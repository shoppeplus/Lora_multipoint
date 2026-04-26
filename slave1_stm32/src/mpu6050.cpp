#include "mpu6050.h"

static void mpuWriteReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void mpuInit() {
    mpuWriteReg(PWR_MGMT_1, 0x00);
    delay(100);
    mpuWriteReg(ACCEL_CONFIG, 0x00);  // ±2g
    mpuWriteReg(SMPLRT_DIV, 0x00);    // 1000 Hz
    mpuWriteReg(MPU_CONFIG, 0x03);    // DLPF ~44Hz
}

void readAccelXYZ(float &ax, float &ay, float &az) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(ACCEL_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6);

    int16_t rawX = ((int16_t)Wire.read() << 8) | Wire.read();
    int16_t rawY = ((int16_t)Wire.read() << 8) | Wire.read();
    int16_t rawZ = ((int16_t)Wire.read() << 8) | Wire.read();

    ax = rawX / 16384.0f;
    ay = rawY / 16384.0f;
    az = rawZ / 16384.0f;
}
