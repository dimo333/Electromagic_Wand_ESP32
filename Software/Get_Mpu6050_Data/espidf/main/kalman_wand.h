/*
 * ------------------------------------------------------------
 * Kalman orientation filter for the wand, ported 1:1 from:
 *   Software/wand_example/wand_example.ino
 *   Original math author: Songyeyaosong (github.com/Songyeyaosong/MagicWand)
 *
 * Fuses accelerometer + gyroscope into roll/pitch, then removes gravity
 * and rotates the acceleration into the global frame, producing the
 * (Ox, Oy, Oz) linear-acceleration signal (in m/s^2) that feeds the model.
 * ------------------------------------------------------------
 */
#pragma once

#include <stdint.h>
#include "mpu6050.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* optimal (Kalman-estimated) angles */
    float k_roll, k_pitch;
    /* error covariance matrix P */
    float e_P[2][2];
    /* Kalman gain K */
    float k_k[2][2];
    /* timestamp of the previous update, microseconds (esp_timer) */
    int64_t prev_time_us;
} wand_kalman_t;

/**
 * Initialise the filter state from the current accelerometer reading.
 * Call this once immediately before each gesture-capture burst
 * (equivalent to resetState() in the .ino).
 */
void wand_kalman_reset(wand_kalman_t *st, mpu6050_handle_t mpu);

/**
 * Read one sample, run one Kalman update step, and output the global-frame
 * linear acceleration in m/s^2.
 *
 * @param ori_out  float[3] receiving {Ox, Oy, Oz} * 9.8
 */
void wand_kalman_step(wand_kalman_t *st, mpu6050_handle_t mpu, float ori_out[3]);

#ifdef __cplusplus
}
#endif
