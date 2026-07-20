/*
 * ------------------------------------------------------------
 * MPU6050 minimal driver for ESP-IDF (new i2c_master API).
 *
 * Ported from the Arduino `MPU6050` library usage in:
 *   Software/wand_example/wand_example.ino
 * of the Electromagic_Wand_ESP32 project.
 *
 * Only the calls used by the wand are implemented:
 *   - initialize (wake device, default ranges: accel +-2g, gyro +-250 dps)
 *   - testConnection (WHO_AM_I)
 *   - getMotion6 / getAcceleration
 *
 * Default sensitivities therefore match the .ino code:
 *   accelerometer: 16384 LSB/g
 *   gyroscope    : 131   LSB/(deg/s)
 * ------------------------------------------------------------
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mpu6050_dev *mpu6050_handle_t;

/**
 * Create the I2C master bus, add the MPU6050 device and wake it up.
 *
 * @param sda_gpio  SDA pin (ESP32-C3 SuperMini default in this project: 8)
 * @param scl_gpio  SCL pin (ESP32-C3 SuperMini default in this project: 9)
 * @param i2c_addr  7-bit address, usually 0x68 (or 0x69 if AD0 is high)
 * @param out       receives the created handle on success
 */
esp_err_t mpu6050_init(int sda_gpio, int scl_gpio, uint8_t i2c_addr,
                       mpu6050_handle_t *out);

/** @return true if WHO_AM_I reports a valid MPU6050. */
bool mpu6050_test_connection(mpu6050_handle_t dev);

/** Read raw accelerometer + gyroscope (6 x int16). Any pointer may be NULL. */
esp_err_t mpu6050_get_motion6(mpu6050_handle_t dev,
                              int16_t *ax, int16_t *ay, int16_t *az,
                              int16_t *gx, int16_t *gy, int16_t *gz);

/** Read raw accelerometer only (3 x int16). */
esp_err_t mpu6050_get_acceleration(mpu6050_handle_t dev,
                                   int16_t *ax, int16_t *ay, int16_t *az);

#ifdef __cplusplus
}
#endif
