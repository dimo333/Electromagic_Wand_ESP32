/*
 * ------------------------------------------------------------
 * ESP-IDF port of Software/wand_example/wand_example.ino
 *
 * Original project : https://github.com/dimo333/Electromagic_Wand_ESP32
 * Derived from     : https://github.com/Songyeyaosong/MagicWand
 *
 * Behaviour is identical to the Arduino sketch:
 *   - press & release the button to capture a (freq * second) sample gesture
 *   - the Kalman-filtered global acceleration is fed to a TFLite Micro model
 *   - the arg-max class triggers an action (LED blink demo for class 0)
 * ------------------------------------------------------------
 */
#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/schema/schema_generated.h"

extern "C" {
#include "mpu6050.h"
#include "kalman_wand.h"
}
#include "model.h"

// =====================================================================
//  USER CONFIGURATION  —  edit these to match your wand & model
// =====================================================================

// ---- Model shape -----------------------------------------------------
// NUM_CLASSES : number of gestures your model was trained on.
// INPUT_DIM   : number of axes fed per sample (2 in the reference model).
// The bundled model.h is Software/models/0922_6 → 6 gestures, y & z axes,
// 100 Hz x 2 s (best accuracy ~95%). See Software/models/README.md.
static constexpr int NUM_CLASSES = 6;
static constexpr int INPUT_DIM   = 2;

// ---- Which global-frame axes to feed the model -----------------------
// wand_kalman_step() outputs {Ox, Oy, Oz} at indices 0,1,2.
// IMPORTANT: these MUST match the axes used when the model was trained
// (see Software/models/README.md). The bundled 0922_6 model uses y & z.
//   0922_6 / 0908_6  → y & z → AXIS_0=1, AXIS_1=2   (current default)
//   1030_5           → x & z → AXIS_0=0, AXIS_1=2   (and 64 Hz, see below)
static constexpr int AXIS_0 = 1;   // -> input[i*INPUT_DIM + 0]  (Oy)
static constexpr int AXIS_1 = 2;   // -> input[i*INPUT_DIM + 1]  (Oz)

// ---- Sampling --------------------------------------------------------
// FREQ x SECOND MUST match the model's timesteps (0922_6/0908_6 = 100 Hz x 2 s
// = 200 steps; 1030_5 = 64 Hz x 2 s = 128 steps).
static constexpr int FREQ   = 100;             // samples per second
static constexpr int SECOND = 2;               // capture length in seconds
static constexpr int NUM_SAMPLES = FREQ * SECOND;

// ---- Pins ------------------------------------------------------------
// ESP32-C3 SuperMini defaults used in this project.
static constexpr int PIN_SDA    = 8;
static constexpr int PIN_SCL    = 9;
static constexpr gpio_num_t PIN_BUTTON = GPIO_NUM_4;   // active-low, INPUT_PULLUP
static constexpr gpio_num_t PIN_LED    = GPIO_NUM_5;

static constexpr uint8_t MPU6050_ADDR = 0x68;

// ---- Tensor arena ----------------------------------------------------
static constexpr int kTensorArenaSize = 50 * 1024;

// =====================================================================

static const char *TAG = "wand";

namespace {
uint8_t             g_tensor_arena[kTensorArenaSize];
tflite::MicroInterpreter *g_interpreter = nullptr;
TfLiteTensor       *g_input  = nullptr;
mpu6050_handle_t    g_mpu    = nullptr;
wand_kalman_t       g_kalman;
}  // namespace

static int64_t millis(void)
{
    return esp_timer_get_time() / 1000;
}

// ------------------------------------------------------------------
//  React to the recognised gesture. Replace the bodies with your own
//  actions (IR, BLE keyboard/mouse, WiFi/MQTT, ...).
// ------------------------------------------------------------------
static void process_gesture(const float *output)
{
    int   max_index = 0;
    float max_value = output[0];
    for (int i = 1; i < NUM_CLASSES; i++) {
        if (output[i] >= max_value) {
            max_value = output[i];
            max_index = i;
        }
    }

    ESP_LOGI(TAG, "gesture = %d (score %.3f)", max_index, max_value);

    if (max_index == 0) {
        // Demo: blink the LED for gesture 0.
        gpio_set_level(PIN_LED, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level(PIN_LED, 0);
        vTaskDelay(pdMS_TO_TICKS(200));
    } else if (max_index == 1) {
        // TODO: your action for gesture 1
    } else if (max_index == 2) {
        // TODO: your action for gesture 2
    } else {
        // TODO: other gestures
    }
}

// ------------------------------------------------------------------
//  Capture one full gesture into the model input tensor.
// ------------------------------------------------------------------
static void capture_gesture(float *input)
{
    wand_kalman_reset(&g_kalman, g_mpu);

    float ori[3];
    for (int i = 0; i < NUM_SAMPLES; i++) {
        wand_kalman_step(&g_kalman, g_mpu, ori);
        input[i * INPUT_DIM + 0] = ori[AXIS_0];
        input[i * INPUT_DIM + 1] = ori[AXIS_1];
        vTaskDelay(pdMS_TO_TICKS(1000 / FREQ));   // ~10 ms @ 100 Hz
    }
}

static bool setup(void)
{
    // ---- TFLite Micro ----
    const tflite::Model *model = tflite::GetModel(model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "Model schema %lu != supported %d",
                 (unsigned long)model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    // Register only the ops used by the reference model (same list as the .ino).
    static tflite::MicroMutableOpResolver<7> resolver;
    resolver.AddConv2D();
    resolver.AddRelu();
    resolver.AddFullyConnected();
    resolver.AddSoftmax();
    resolver.AddReshape();
    resolver.AddTranspose();
    resolver.AddExpandDims();

    static tflite::MicroInterpreter interpreter(
        model, resolver, g_tensor_arena, kTensorArenaSize);
    g_interpreter = &interpreter;

    if (g_interpreter->AllocateTensors() != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors() failed");
        return false;
    }
    g_input = g_interpreter->input(0);

    // ---- MPU6050 ----
    esp_err_t err = mpu6050_init(PIN_SDA, PIN_SCL, MPU6050_ADDR, &g_mpu);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mpu6050_init failed: %s", esp_err_to_name(err));
        return false;
    }
    if (!mpu6050_test_connection(g_mpu)) {
        ESP_LOGE(TAG, "MPU6050 connection failed");   // matches "MPU6050连接失败"
        return false;
    }

    // ---- GPIO ----
    gpio_config_t btn = {
        .pin_bit_mask = 1ULL << PIN_BUTTON,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn);

    gpio_config_t led = {
        .pin_bit_mask = 1ULL << PIN_LED,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&led);
    gpio_set_level(PIN_LED, 0);

    wand_kalman_reset(&g_kalman, g_mpu);
    ESP_LOGI(TAG, "wand ready — press & release the button to cast");
    return true;
}

// ------------------------------------------------------------------
//  Main loop: debounced button, capture on release, then infer.
//  Mirrors loop() in wand_example.ino.
// ------------------------------------------------------------------
static void loop_task(void *arg)
{
    int   button_state       = 1;
    int   last_button_state  = 1;      // HIGH (released, pull-up)
    int64_t last_debounce_ms = 0;
    const int64_t debounce_delay_ms = 10;

    for (;;) {
        int reading = gpio_get_level(PIN_BUTTON);

        if (reading != last_button_state) {
            last_debounce_ms = millis();
        }

        if ((millis() - last_debounce_ms) > debounce_delay_ms) {
            if (reading != button_state) {
                button_state = reading;

                // Act on the press→release edge (button goes HIGH).
                if (button_state == 1) {
                    capture_gesture(g_input->data.f);

                    if (g_interpreter->Invoke() != kTfLiteOk) {
                        ESP_LOGE(TAG, "Invoke failed");
                    } else {
                        process_gesture(g_interpreter->output(0)->data.f);
                    }
                }
            }
        }

        last_button_state = reading;
        vTaskDelay(pdMS_TO_TICKS(1));   // yield; keeps the debounce responsive
    }
}

extern "C" void app_main(void)
{
    if (!setup()) {
        ESP_LOGE(TAG, "setup failed — halting");
        return;
    }
    xTaskCreate(loop_task, "wand_loop", 8192, nullptr, 5, nullptr);
}
