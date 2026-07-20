/*
 * ------------------------------------------------------------
 * Kalman orientation filter for the wand. See kalman_wand.h.
 * The math is a direct port of get_kalman_mpu_data()/resetState()
 * in Software/wand_example/wand_example.ino.
 * ------------------------------------------------------------
 */
#include "kalman_wand.h"

#include <math.h>
#include "esp_timer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Measurement-noise R and process-noise Q from the original code. */
#define KALMAN_Q 0.0025f
#define KALMAN_R 0.3f

void wand_kalman_reset(wand_kalman_t *st, mpu6050_handle_t mpu)
{
    int16_t ax, ay, az;
    mpu6050_get_acceleration(mpu, &ax, &ay, &az);

    float Ax = ax / 16384.0f;
    float Ay = ay / 16384.0f;
    float Az = az / 16384.0f;

    /* seed the estimate from the accelerometer-only angles */
    st->k_pitch = -atan2f(Ax, sqrtf(Ay * Ay + Az * Az));
    st->k_roll  =  atan2f(Ay, Az);

    st->e_P[0][0] = 1.0f;
    st->e_P[0][1] = 0.0f;
    st->e_P[1][0] = 0.0f;
    st->e_P[1][1] = 1.0f;

    st->k_k[0][0] = 0.0f;
    st->k_k[0][1] = 0.0f;
    st->k_k[1][0] = 0.0f;
    st->k_k[1][1] = 0.0f;

    st->prev_time_us = esp_timer_get_time();
}

void wand_kalman_step(wand_kalman_t *st, mpu6050_handle_t mpu, float ori_out[3])
{
    /* differential time in seconds */
    int64_t now_us = esp_timer_get_time();
    float dt = (now_us - st->prev_time_us) / 1000000.0f;
    st->prev_time_us = now_us;

    int16_t ax, ay, az, gx, gy, gz;
    mpu6050_get_motion6(mpu, &ax, &ay, &az, &gx, &gy, &gz);

    float Ax = ax / 16384.0f;
    float Ay = ay / 16384.0f;
    float Az = az / 16384.0f;

    /* gyro raw -> rad/s */
    float Gx = gx / 131.0f / 180.0f * (float)M_PI;
    float Gy = gy / 131.0f / 180.0f * (float)M_PI;
    float Gz = gz / 131.0f / 180.0f * (float)M_PI;

    float k_roll  = st->k_roll;
    float k_pitch = st->k_pitch;

    /* step 1: a-priori (gyro-integrated) angles */
    float roll_v  = Gx + ((sinf(k_pitch) * sinf(k_roll)) / cosf(k_pitch)) * Gy
                       + ((sinf(k_pitch) * cosf(k_roll)) / cosf(k_pitch)) * Gz;
    float pitch_v = cosf(k_roll) * Gy - sinf(k_roll) * Gz;
    float gyro_roll  = k_roll  + dt * roll_v;
    float gyro_pitch = k_pitch + dt * pitch_v;

    /* step 2: a-priori error covariance (Q is diagonal, 0.0025) */
    st->e_P[0][0] += KALMAN_Q;
    st->e_P[1][1] += KALMAN_Q;

    /* step 3: Kalman gain */
    st->k_k[0][0] = st->e_P[0][0] / (st->e_P[0][0] + KALMAN_R);
    st->k_k[0][1] = 0.0f;
    st->k_k[1][0] = 0.0f;
    st->k_k[1][1] = st->e_P[1][1] / (st->e_P[1][1] + KALMAN_R);

    /* step 4: correct with accelerometer observation */
    float acc_roll  =  atan2f(Ay, Az);
    float acc_pitch = -atan2f(Ax, sqrtf(Ay * Ay + Az * Az));
    k_roll  = gyro_roll  + st->k_k[0][0] * (acc_roll  - gyro_roll);
    k_pitch = gyro_pitch + st->k_k[1][1] * (acc_pitch - gyro_pitch);
    st->k_roll  = k_roll;
    st->k_pitch = k_pitch;

    /* step 5: update covariance */
    st->e_P[0][0] = (1.0f - st->k_k[0][0]) * st->e_P[0][0];
    st->e_P[0][1] = 0.0f;
    st->e_P[1][0] = 0.0f;
    st->e_P[1][1] = (1.0f - st->k_k[1][1]) * st->e_P[1][1];

    /* gravity direction from the estimated attitude */
    float gravity_x = -sinf(k_pitch);
    float gravity_y =  sinf(k_roll) * cosf(k_pitch);
    float gravity_z =  cosf(k_roll) * cosf(k_pitch);

    /* remove gravity */
    Ax -= gravity_x;
    Ay -= gravity_y;
    Az -= gravity_z;

    /* rotate into the global frame */
    float Ox = cosf(k_pitch) * Ax
             + sinf(k_pitch) * sinf(k_roll) * Ay
             + sinf(k_pitch) * cosf(k_roll) * Az;
    float Oy = cosf(k_roll) * Ay - sinf(k_roll) * Az;
    float Oz = -sinf(k_pitch) * Ax
             +  cosf(k_pitch) * sinf(k_roll) * Ay
             +  cosf(k_pitch) * cosf(k_roll) * Az;

    ori_out[0] = Ox * 9.8f;
    ori_out[1] = Oy * 9.8f;
    ori_out[2] = Oz * 9.8f;
}
