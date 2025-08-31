#include <TensorFlowLite_ESP32.h>
#include "model.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <Wire.h>
#include <MPU6050.h>

namespace {
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* model_input = nullptr;

  const int num_classes = 5;
  const int input_dim = 2;

  constexpr int kTensorArenaSize = 50 * 1024;
  uint8_t tensor_arena[kTensorArenaSize];

  MPU6050 mpu;
  unsigned long prevTime;
  const int freq = 100;   // 每秒采样次数
  const int second = 2;   // 采样时长（秒）
  const int buttonPin = 7;
  const int ledPin = 1;
  const int irReceiverPin = 6;
  const int IREMITTERPIN = 4;
  const int ledChannel = 0;
  const int frequency = 38000;
  const int resolution = 8;

  bool IRNOW = false;
  bool IRGET = false;
  bool buttonPressed = false;
  int buttonState, lastButtonState = HIGH;
  unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 10;
  volatile unsigned long lastChangeTime = 0;
  volatile unsigned long pulseWidth;
  volatile enum { EMPTY, RECORDING, READY } state = EMPTY;
  volatile unsigned long irBuffer[400];
  volatile int irIndex = 0;
}

float gravity_x, gravity_y, gravity_z;
float roll_v, pitch_v;
float gyro_roll, gyro_pitch;
float acc_roll, acc_pitch;
float k_roll, k_pitch;
float e_P[2][2];
float k_k[2][2];

void setup() {
  Serial.begin(115200);
  pinMode(irReceiverPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(IREMITTERPIN, OUTPUT);
  ledcSetup(ledChannel, frequency, resolution);
  ledcAttachPin(IREMITTERPIN, ledChannel);
  attachInterrupt(digitalPinToInterrupt(irReceiverPin), handleInterrupt, CHANGE);

  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(model_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report("Model version mismatch");
    return;
  }

  static tflite::AllOpsResolver resolver;
  resolver.AddConv2D();
  resolver.AddRelu();
  resolver.AddFullyConnected();
  resolver.AddSoftmax();
  resolver.AddReshape();
  resolver.AddTranspose();
  resolver.AddExpandDims();

  static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;
  interpreter->AllocateTensors();
  model_input = interpreter->input(0);

  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  resetState();
}

void loop() {
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        Serial.println("Button pressed");
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

  e_P[0][0] = e_P[1][1] = 1;
  e_P[0][1] = e_P[1][0] = 0;

  k_k[0][0] = k_k[1][1] = 0;
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
  float Gx = gx / 131.0 / 180 * PI;
  float Gy = gy / 131.0 / 180 * PI;
  float Gz = gz / 131.0 / 180 * PI;

  roll_v = Gx + ((sin(k_pitch) * sin(k_roll)) / cos(k_pitch)) * Gy + ((sin(k_pitch) * cos(k_roll)) / cos(k_pitch)) * Gz;
  pitch_v = cos(k_roll) * Gy - sin(k_roll) * Gz;
  gyro_roll = k_roll + dt * roll_v;
  gyro_pitch = k_pitch + dt * pitch_v;

  e_P[0][0] += 0.0025;
  e_P[1][1] += 0.0025;

  k_k[0][0] = e_P[0][0] / (e_P[0][0] + 0.3);
  k_k[1][1] = e_P[1][1] / (e_P[1][1] + 0.3);

  acc_roll = atan2(Ay, Az);
  acc_pitch = -atan2(Ax, sqrt(Ay * Ay + Az * Az));

  k_roll = gyro_roll + k_k[0][0] * (acc_roll - gyro_roll);
  k_pitch = gyro_pitch + k_k[1][1] * (acc_pitch - gyro_pitch);

  e_P[0][0] *= (1 - k_k[0][0]);
  e_P[1][1] *= (1 - k_k[1][1]);

  gravity_x = -sin(k_pitch);
  gravity_y = sin(k_roll) * cos(k_pitch);
  gravity_z = cos(k_roll) * cos(k_pitch);

  Ax -= gravity_x;
  Ay -= gravity_y;
  Az -= gravity_z;

  float Ox = cos(k_pitch) * Ax + sin(k_pitch) * sin(k_roll) * Ay + sin(k_pitch) * cos(k_roll) * Az;
  float Oz = -sin(k_pitch) * Ax + cos(k_pitch) * sin(k_roll) * Ay + cos(k_pitch) * cos(k_roll) * Az;

  input[i * input_dim] = Ox;
  input[i * input_dim + 1] = Oz;

  delay(1000 / freq);
}

void processGesture(float* output) {
  int max_index = 0;
  float max_value = output[0];

  for (int i = 1; i < num_classes; i++) {
    if (output[i] > max_value) {
      max_value = output[i];
      max_index = i;
    }
  }

  if (max_index == 0) {
    Serial.println("_UpAndDown_");
   
      irsend(); // 发送红外信号
    
  } else if (max_index == 1) {
    Serial.println("_RightAngle_");
  } else if (max_index == 2) {
    Serial.println("_Letter_W_");
  } else if (max_index == 3) {
    Serial.println("_SharpAngle_");
  } else if (max_index == 4) {
    Serial.println("_Horn_");
     if(IRNOW == false){
      IRGET = true; // 设置接收模式
      IRNOW = true; // 标记正在接收
    }
  }
}

void irsend() {
  detachInterrupt(digitalPinToInterrupt(irReceiverPin));

  Serial.println("Sending IR signal");

  for (int i = 0; i < irIndex; i++) {
    ledcWrite(ledChannel, (i % 2 == 0) ? 128 : 0);
    delayMicroseconds(irBuffer[i]);
  }

  ledcWrite(ledChannel, 0);
  // 打印接收到的数据
  for (int i = 0; i < irIndex; i++) {
    Serial.print(i);
    Serial.print("-");
    Serial.print(irBuffer[i]);
    Serial.println(); 
  }
  attachInterrupt(digitalPinToInterrupt(irReceiverPin), handleInterrupt, CHANGE);
}

void handleInterrupt() {
  static bool firstChange = true;
  unsigned long currentTime = micros();

  if (IRGET) {
    if (firstChange) {
      state = RECORDING;
      firstChange = false;
      lastChangeTime = currentTime;
    } else {
      pulseWidth = currentTime - lastChangeTime;
      if (pulseWidth > 50000) {
        state = READY;
      } else if (pulseWidth > 400 && state == RECORDING && irIndex < 400) {
        irBuffer[irIndex++] = pulseWidth;
      }
      lastChangeTime = currentTime;
    }
  }
}
