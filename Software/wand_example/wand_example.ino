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

const int num_classes = 4;//////////////////////////////////////////////////////////////动作（手势）的数量，有几个就写几个
const int input_dim = 2;////////////////////////////////////////////////////////////////传感器收集数据的维度，比如你只录入了x轴和z轴，那就写2，类推。

constexpr int kTensorArenaSize = 50 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
}

MPU6050 mpu;

// 定义每秒采样次数
const int freq = 100;
const int second = 2;
// const int freq = 64;
// const int second = 2;

// 重力分量
float gravity_x;
float gravity_y;
float gravity_z;

// 换算到x,y轴上的角速度
float roll_v, pitch_v;

// 上次更新时间
unsigned long prevTime;

// 三个状态，先验状态，观测状态，最优估计状态
float gyro_roll, gyro_pitch;        //陀螺仪积分计算出的角度，先验状态
float acc_roll, acc_pitch;          //加速度计观测出的角度，观测状态
float k_roll, k_pitch;              //卡尔曼滤波后估计出最优角度，最优估计状态

// 误差协方差矩阵P
float e_P[2][2];         //误差协方差矩阵，这里的e_P既是先验估计的P，也是最后更新的P

// 卡尔曼增益K
float k_k[2][2];         //这里的卡尔曼增益矩阵K是一个2X2的方阵

const int buttonPin = 4; /////////////////////////////////////////////////////////////////////////填写你想要定义按钮的引脚
const int ledPin = 12;   /////////////////////////////////////////////////////////////////////////填写你想要控制点亮led的引脚，你也可以不使用点灯示例
int buttonState;          // 当前按钮状态
int lastButtonState = HIGH; // 上一次按钮状态

unsigned long lastDebounceTime = 0; // 上一次去抖动时间
unsigned long debounceDelay = 10;   // 去抖动延时

void setup() {
  Serial.begin(115200);

  static tflite::MicroErrorReporter micro_error_reporter;  // NOLINT
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(model_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report(
      "Model provided is schema version %d not equal "
      "to supported version %d.",
      model->version(), TFLITE_SCHEMA_VERSION);
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

  static tflite::MicroInterpreter static_interpreter(
    model, resolver, tensor_arena, kTensorArenaSize,
    error_reporter);
  interpreter = &static_interpreter;

  interpreter->AllocateTensors();
  model_input = interpreter->input(0);

  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050连接失败");
    while (1);
  }

  pinMode(buttonPin, INPUT_PULLUP); // 将按钮引脚设置为输入模式，并启用内部上拉电阻
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  resetState();
}

void loop() {

  int reading = digitalRead(buttonPin); // 读取按钮引脚的电平状态

  // 检查是否有按钮状态变化
  if (reading != lastButtonState) {
    lastDebounceTime = millis(); // 记录状态变化的时间
  }

  // 如果状态变化超过去抖动延时，认为是有效变化
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // 如果按钮状态确实变化了
    if (reading != buttonState) {
      buttonState = reading;

      // 只有在按钮从按下变为释放时，才改变LED状态
      if (buttonState == HIGH) {
        resetState();

        for (int i = 0; i < freq * second; i ++) {
          get_kalman_mpu_data(i, model_input->data.f);
        }

        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk) {
          error_reporter->Report("Invoke failed");
          return;
        }

        processGesture(interpreter->output(0)->data.f);
      }
    }
  }

  // 记录上一次按钮状态
  lastButtonState = reading;
}

void resetState() {
  // 读取加速度计数据
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  // 转换加速度计数据为g值
  float Ax = ax / 16384.0;
  float Ay = ay / 16384.0;
  float Az = az / 16384.0;

  // 计算Pitch和Roll
  k_pitch = -atan2(Ax, sqrt(Ay * Ay + Az * Az));
  k_roll = atan2(Ay, Az);

  // 误差协方差矩阵P
  e_P[0][0] = 1;
  e_P[0][1] = 0;
  e_P[1][0] = 0;
  e_P[1][1] = 1;

  // 卡尔曼增益K
  k_k[0][0] = 0;
  k_k[0][0] = 0;
  k_k[0][0] = 0;
  k_k[0][0] = 0;

  prevTime = millis();
}

void get_kalman_mpu_data(int i, float* input) {
  // 计算微分时间
  unsigned long currentTime = millis();
  float dt = (currentTime - prevTime) / 1000.0; // 时间间隔（秒）
  prevTime = currentTime;

  // 获取角速度和加速度
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // 转换加速度计数据为g值
  float Ax = ax / 16384.0;
  float Ay = ay / 16384.0;
  float Az = az / 16384.0;
  float Ox, Oy, Oz;

  // 转换陀螺仪数据为弧度/秒
  float Gx = gx / 131.0 / 180 * PI;
  float Gy = gy / 131.0 / 180 * PI;
  float Gz = gz / 131.0 / 180 * PI;

  // step1:计算先验状态
  // 计算roll, pitch, yaw轴上的角速度
  roll_v = Gx + ((sin(k_pitch) * sin(k_roll)) / cos(k_pitch)) * Gy + ((sin(k_pitch) * cos(k_roll)) / cos(k_pitch)) * Gz; //roll轴的角速度
  pitch_v = cos(k_roll) * Gy - sin(k_roll) * Gz; //pitch轴的角速度
  gyro_roll = k_roll + dt * roll_v; //先验roll角度
  gyro_pitch = k_pitch + dt * pitch_v; //先验pitch角度

  // step2:计算先验误差协方差矩阵
  e_P[0][0] = e_P[0][0] + 0.0025;//这里的Q矩阵是一个对角阵且对角元均为0.0025
  e_P[0][1] = e_P[0][1] + 0;
  e_P[1][0] = e_P[1][0] + 0;
  e_P[1][1] = e_P[1][1] + 0.0025;

  // step3:更新卡尔曼增益
  k_k[0][0] = e_P[0][0] / (e_P[0][0] + 0.3);
  k_k[0][1] = 0;
  k_k[1][0] = 0;
  k_k[1][1] = e_P[1][1] / (e_P[1][1] + 0.3);

  // step4:计算最优估计状态
  // 观测状态
  // roll角度
  acc_roll = atan2(Ay, Az);
  //pitch角度
  acc_pitch = -atan2(Ax, sqrt(Ay * Ay + Az * Az));
  // 最优估计状态
  k_roll = gyro_roll + k_k[0][0] * (acc_roll - gyro_roll);
  k_pitch = gyro_pitch + k_k[1][1] * (acc_pitch - gyro_pitch);

  // step5:更新协方差矩阵
  e_P[0][0] = (1 - k_k[0][0]) * e_P[0][0];
  e_P[0][1] = 0;
  e_P[1][0] = 0;
  e_P[1][1] = (1 - k_k[1][1]) * e_P[1][1];

  // 计算重力加速度方向
  gravity_x = -sin(k_pitch);
  gravity_y = sin(k_roll) * cos(k_pitch);
  gravity_z = cos(k_roll) * cos(k_pitch);

  // 重力消除
  Ax = Ax - gravity_x;
  Ay = Ay - gravity_y;
  Az = Az - gravity_z;

  // 得到全局空间坐标系中的相对加速度
  Ox = cos(k_pitch) * Ax + sin(k_pitch) * sin(k_roll) * Ay + sin(k_pitch) * cos(k_roll) * Az;
  Oy = cos(k_roll) * Ay - sin(k_roll) * Az;
  Oz = -sin(k_pitch) * Ax + cos(k_pitch) * sin(k_roll) * Ay + cos(k_pitch) * cos(k_roll) * Az;

  input[i * input_dim] = Ox*9.8;
  input[i * input_dim + 1] = Oz*9.8;

  delay(1000 / freq); // 短暂延迟，避免过高的循环频率
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

  if (max_index == 0) {       ///////////////////////////////////////////////这里就是对应动作0的情况，我在这里简单的写入了一个闪灯的示例，你可以根据自己的需求来进行修改
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(200);
//  } else if (max_index == 1) {
//    
//  } else if (max_index == 2) {
//    
//  } else {
    
  }
}

