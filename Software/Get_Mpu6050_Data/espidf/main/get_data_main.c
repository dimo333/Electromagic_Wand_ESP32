/*
 * ------------------------------------------------------------
 * ESP-IDF port of Software/Get_Mpu6050_Data/arduino/Get_Mpu6050_Data.ino
 *
 * Original project : https://github.com/dimo333/Electromagic_Wand_ESP32
 * Derived from     : https://github.com/Songyeyaosong/MagicWand
 *
 * Same logic as the Arduino sketch: press & release the button to capture a
 * (FREQ * SECOND) sample gesture; each sample is Kalman-filtered and gravity-
 * removed, then the two selected global-frame axes are printed over the serial
 * console as one comma-separated line — ready for Tensorflow_model_train.
 * ------------------------------------------------------------
 */
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "mpu6050.h"
#include "kalman_wand.h"

// =====================================================================
//  USER CONFIGURATION  —  keep these identical to your wand firmware!
// =====================================================================

// ---- Sampling (must match the model you will train / deploy) ----------
#define FREQ         100                 // samples per second
#define SECOND       2                   // capture length in seconds
#define NUM_SAMPLES  (FREQ * SECOND)

// ---- Which global-frame axes to record --------------------------------
// wand_kalman_step() outputs {Ox, Oy, Oz} at indices 0,1,2.
// These MUST match the axes fed to the model in the wand firmware, and the
// axes used to train it (see Software/models/README.md).
//   y & z (0922_6 / 0908_6, current default) → AXIS_0=1, AXIS_1=2
//   x & z (old 1030_5 / Arduino sketch)       → AXIS_0=0, AXIS_1=2
#define AXIS_0       1                   // -> first  value per sample (Oy)
#define AXIS_1       2                   // -> second value per sample (Oz)

// ---- Pins (ESP32-C3 SuperMini defaults, same as the wand) -------------
#define PIN_SDA      8
#define PIN_SCL      9
#define PIN_BUTTON   GPIO_NUM_4          // active-low, INPUT_PULLUP
#define MPU6050_ADDR 0x68

// =====================================================================

static const char *TAG = "getdata";

static mpu6050_handle_t g_mpu = NULL;
static wand_kalman_t    g_kalman;

static int64_t millis(void)
{
    return esp_timer_get_time() / 1000;
}

// Capture one gesture and print it as a single CSV line on the console:
//   a0,b0,a1,b1,...,a(N-1),b(N-1)\n
static void capture_and_print(void)
{
    wand_kalman_reset(&g_kalman, g_mpu);

    float ori[3];
    for (int i = 0; i < NUM_SAMPLES; i++) {
        wand_kalman_step(&g_kalman, g_mpu, ori);

        printf("%.2f,%.2f", ori[AXIS_0], ori[AXIS_1]);
        if (i != NUM_SAMPLES - 1) {
            printf(",");
        }

        vTaskDelay(pdMS_TO_TICKS(1000 / FREQ));   // ~10 ms @ 100 Hz
    }
    printf("\n");
    fflush(stdout);
}

static bool setup(void)
{
    esp_err_t err = mpu6050_init(PIN_SDA, PIN_SCL, MPU6050_ADDR, &g_mpu);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mpu6050_init failed: %s", esp_err_to_name(err));
        return false;
    }
    if (!mpu6050_test_connection(g_mpu)) {
        ESP_LOGE(TAG, "MPU6050 connection failed");   // matches "MPU6050连接失败"
        return false;
    }

    gpio_config_t btn = {
        .pin_bit_mask = 1ULL << PIN_BUTTON,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn);

    wand_kalman_reset(&g_kalman, g_mpu);
    ESP_LOGI(TAG, "ready — press & release the button to record one gesture");
    return true;
}

// Debounced button; capture on the press→release edge. Mirrors loop() in the
// Arduino sketch.
static void loop_task(void *arg)
{
    int     button_state      = 1;
    int     last_button_state = 1;      // HIGH (released, pull-up)
    int64_t last_debounce_ms  = 0;
    const int64_t debounce_delay_ms = 10;

    for (;;) {
        int reading = gpio_get_level(PIN_BUTTON);

        if (reading != last_button_state) {
            last_debounce_ms = millis();
        }

        if ((millis() - last_debounce_ms) > debounce_delay_ms) {
            if (reading != button_state) {
                button_state = reading;
                if (button_state == 1) {      // released → capture
                    capture_and_print();
                }
            }
        }

        last_button_state = reading;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void app_main(void)
{
    if (!setup()) {
        ESP_LOGE(TAG, "setup failed — halting");
        return;
    }
    xTaskCreate(loop_task, "getdata_loop", 4096, NULL, 5, NULL);
}
