#include <WiFi.h>
#include <PubSubClient.h>
#include <TensorFlowLite_ESP32.h>
#include "model.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <Wire.h>
#include <MPU6050.h>
#include <Ticker.h>
//到手按需修改20,29,30，48,49，56~61，248~261内容
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* model_input = nullptr;

const int num_classes = 4;
const int input_dim = 2;
constexpr int kTensorArenaSize = 50 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
}

MPU6050 mpu;

const int freq = 100;
const int second = 2;

float gravity_x, gravity_y, gravity_z;

float roll_v, pitch_v;

unsigned long prevTime;

float gyro_roll, gyro_pitch;
float acc_roll, acc_pitch;
float k_roll, k_pitch;
float e_P[2][2];
float k_k[2][2];

const int buttonPin = 4;
const int ledPin = 10;
int buttonState;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 10;

// WiFi和MQTT连接信息
const char* ssid = "559";
const char* password = "77777777";
const char* mqttServer = "120.53.106.11";
const int mqttPort = 1883;
const char* mqttUser = "admin";
const char* mqttPassword = "public";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
}
// MQTT客户端信息
void connectToMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT...");
    if (mqttClient.connect("ESP32_C3", mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.print("failed with state ");
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  model = tflite::GetModel(model_tflite);

  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report("Model schema version mismatch");
    return;
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
    model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;
  interpreter->AllocateTensors();
  model_input = interpreter->input(0);

  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  resetState();

  connectToWiFi();
  mqttClient.setServer(mqttServer, mqttPort);
  connectToMQTT();
}

void loop() {
  if (!mqttClient.connected()) {
    connectToMQTT();
  }
  mqttClient.loop();

  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        Serial.println("请开始动作");
        resetState();

        for (int i = 0; i < freq * second; i++) {
          get_kalman_mpu_data(i, model_input->data.f);
        }

        if (interpreter->Invoke() != kTfLiteOk) {
          error_reporter->Report("Invoke failed");
          return;
        }

        processGesture(interpreter->output(0)->data.f);
      }
    }
  }
  lastButtonState = reading;
}

void resetState() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  float Ax = ax / 16384.0;
  float Ay = ay / 16384.0;
  float Az = az / 16384.0;

  k_pitch = -atan2(Ax, sqrt(Ay * Ay + Az * Az));
  k_roll = atan2(Ay, Az);

  e_P[0][0] = 1;
  e_P[0][1] = 0;
  e_P[1][0] = 0;
  e_P[1][1] = 1;

  k_k[0][0] = 0;
  k_k[0][0] = 0;
  k_k[0][0] = 0;
  k_k[0][0] = 0;

  prevTime = millis();
}

void get_kalman_mpu_data(int i, float* input) {
  unsigned long currentTime = millis();
  float dt = (currentTime - prevTime) / 1000.0;
  prevTime = currentTime;

  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float Ax = ax / 16384.0;
  float Ay = ay / 16384.0;
  float Az = az / 16384.0;
  float Ox, Oy, Oz;

  float Gx = gx / 131.0 / 180 * PI;
  float Gy = gy / 131.0 / 180 * PI;
  float Gz = gz / 131.0 / 180 * PI;

  roll_v = Gx + ((sin(k_pitch) * sin(k_roll)) / cos(k_pitch)) * Gy + ((sin(k_pitch) * cos(k_roll)) / cos(k_pitch)) * Gz;
  pitch_v = cos(k_roll) * Gy - sin(k_roll) * Gz;
  gyro_roll = k_roll + dt * roll_v;
  gyro_pitch = k_pitch + dt * pitch_v;

  e_P[0][0] = e_P[0][0] + 0.0025;
  e_P[1][1] = e_P[1][1] + 0.0025;

  k_k[0][0] = e_P[0][0] / (e_P[0][0] + 0.3);
  k_k[1][1] = e_P[1][1] / (e_P[1][1] + 0.3);

  acc_roll = atan2(Ay, Az);
  acc_pitch = -atan2(Ax, sqrt(Ay * Ay + Az * Az));
  k_roll = gyro_roll + k_k[0][0] * (acc_roll - gyro_roll);
  k_pitch = gyro_pitch + k_k[1][1] * (acc_pitch - gyro_pitch);

  e_P[0][0] = (1 - k_k[0][0]) * e_P[0][0];
  e_P[1][1] = (1 - k_k[1][1]) * e_P[1][1];

  gravity_x = -sin(k_pitch);
  gravity_y = sin(k_roll) * cos(k_pitch);
  gravity_z = cos(k_roll) * cos(k_pitch);

  Ax -= gravity_x;
  Ay -= gravity_y;
  Az -= gravity_z;

  Ox = cos(k_pitch) * Ax + sin(k_pitch) * sin(k_roll) * Ay + sin(k_pitch) * cos(k_roll) * Az;
  Oy = cos(k_roll) * Ay - sin(k_roll) * Az;
  Oz = -sin(k_pitch) * Ax + cos(k_pitch) * sin(k_roll) * Ay + cos(k_pitch) * cos(k_roll) * Az;

  input[i * input_dim] = Ox;
  input[i * input_dim + 1] = Oz;

  delay(1000 / freq);
}

void processGesture(float* output) {
  int max_index = 0;
  float max_value = output[0];

  for (int i = 1; i < num_classes; i++) {
    if (output[i] >= max_value) {
      max_value = output[i];
      max_index = i;
    }
  }

  String message;
  if (max_index == 1) {
    message = "1";
  } else if (max_index == 2) {
    message = "2";
  } else if (max_index == 3) {
    message = "3";
  } 

  Serial.println(message);
  mqttClient.publish("wandtx", message.c_str());
}
