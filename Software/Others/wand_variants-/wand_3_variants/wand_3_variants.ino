#include <TensorFlowLite_ESP32.h>
#include "model.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <Wire.h>
#include <MPU6050.h>

//v5.0新增灯效库
#include <Adafruit_NeoPixel.h>

namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* model_input = nullptr;


// ////////////////////////////////////////////////////////////////////////////////////////
const int num_classes = 6;  //动作（手势）的数量
const int input_dim = 2;    //传感器收集数据的维度
// ////////////////////////////////////////////////////////////////////////////////////////


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


// ////////////////////////////////////////////////////////////////////////////////////////
const int buttonPin = 7; //定义按钮的引脚
const int Led_PIN0 = 0;   //led
const int IO_PIN2 = 2;  
const int IO_PIN4 = 4;
const int IO_PIN5 = 5;
const int IO_PIN6 = 6;  
const int Motor_PIN10 = 10;  //motor
// ////////////////////////////////////////////////////////////////////////////////////////





int buttonState = HIGH;      // 当前按钮状态，初始化为 HIGH
int lastButtonState = HIGH;  // 上一次按钮状态

unsigned long lastDebounceTime = 0; // 上一次去抖动时间
unsigned long debounceDelay = 10;   // 去抖动延时


// 全局变量-阈值-置信度（识别率）
float CONFIDENCE_THRESHOLD = 0.75;

// 全局变量-灯珠开关切换状态
bool switchState = false;  // false=关, true=开

// 全局变量-灯带灯效参数
#define NUM_LEDS 40///////////////////////////////////////////////////这里填写你想要控制灯的数量
#define DATA_PIN 1///////////////////////////////////////////////////这里填写你想要控制灯的引脚

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);









void setup() {
  
  Serial.begin(115200);

  //灯带效果初始化
  strip.begin();
  strip.show();



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
//    Serial.println("MPU6050连接失败");
    while (1);
  }

// ////////////////////////////////////////////////////////////////////////////////////////

  pinMode(buttonPin, INPUT_PULLUP); // 将按钮引脚设置为输入模式，并启用内部上拉电阻

  pinMode(Led_PIN0, OUTPUT);         //led
  digitalWrite(Led_PIN0, LOW);

  pinMode(IO_PIN2, OUTPUT);
  digitalWrite(IO_PIN2, LOW);

  pinMode(IO_PIN4, OUTPUT);
  digitalWrite(IO_PIN4, LOW);

  pinMode(IO_PIN5, OUTPUT);
  digitalWrite(IO_PIN5, LOW);

  pinMode(IO_PIN6, OUTPUT);
  digitalWrite(IO_PIN6, LOW);

  pinMode(Motor_PIN10, OUTPUT);     //motor
  digitalWrite(Motor_PIN10, LOW);

// ////////////////////////////////////////////////////////////////////////////////////////
  resetState();

  // 打印模型信息
//  Serial.print("模型地址: ");
//  Serial.println((uintptr_t)model_tflite, HEX);
//  Serial.print("模型长度: ");
//  Serial.println(model_tflite_len);

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

      // 只有在按钮从按下变为释放时，才开始主流程
      if (buttonState == HIGH) {

        //震动马达震动
        digitalWrite(Motor_PIN10, HIGH);
        delay(200);
        digitalWrite(Motor_PIN10, LOW);
        
//        delay(200);
//        digitalWrite(IO_PIN5, HIGH);
//        delay(600);
//        digitalWrite(IO_PIN5, LOW);

        resetState();

        for (int i = 0; i < freq * second; i++) {
          get_kalman_mpu_data(i, model_input->data.f);
        }

        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk) {
          error_reporter->Report("Invoke failed");
          return;
        }

        // ====== 新增部分：带置信度判断 ======
        float* output = interpreter->output(0)->data.f;
        float confidence;
        int gesture = processGesture(output, &confidence);

        if (confidence >= CONFIDENCE_THRESHOLD) {
//          Serial.print("识别成功: 手势 = ");
//          Serial.print(gesture);
//          Serial.print("，置信度 = ");
//          Serial.println(confidence, 2);

          //震动马达震动0.2s----识别成功震动
          digitalWrite(Motor_PIN10, HIGH);
          delay(10);
          
          energyGatherAndBurst(30, 10); //灯带效果-调整这些值来控制效果速度-耗时2.1s

          digitalWrite(Motor_PIN10, LOW);
          delay(10);


          executeGesture(gesture);  // 执行动作

        } else {
//          Serial.print("识别失败，置信度 = ");
//          Serial.println(confidence, 2);

          //震动马达震动0.2s*2=0.5s----识别失败震动
          digitalWrite(Motor_PIN10, HIGH);
          delay(200);
          digitalWrite(Motor_PIN10, LOW);
          delay(100);
          digitalWrite(Motor_PIN10, HIGH);
          delay(200);
          digitalWrite(Motor_PIN10, LOW);
          delay(10);


        }
        // ====== 到这里结束 ======
      }
    }
  }

  // 记录上一次按钮状态
  lastButtonState = reading;
}


//  灯带效果函数
void energyGatherAndBurst(uint8_t gatherDelay, uint8_t burstDelay) {
  int baseLeds = 10;  // 前10个灯

  // 聚集效果 - 前10个LED依次点亮
  for (int i = 0; i < baseLeds; i++) {
    for (int brightness = 0; brightness <= 255; brightness += 50) {
      strip.setPixelColor(i, strip.Color(100, 100, brightness));  //这三个值就是蓄力的参数-目前蓝
      strip.show();
      delay(gatherDelay);
    }
  }

  // 爆发效果 - LED从底部快速移动到顶部
  strip.clear();
  for (int i = 0; i < baseLeds; i++) {
    strip.setPixelColor(i, strip.Color(100, 100, 255));
  }
  strip.show();
  delay(burstDelay);

  for (int i = 1; i <= strip.numPixels() - baseLeds; i++) {
    strip.clear();
    for (int j = 0; j < baseLeds; j++) {
      strip.setPixelColor(i + j, strip.Color(100, 100, 255));
    }
    strip.show();
    delay(burstDelay);
  }

  strip.clear();
  strip.show();
  delay(10);
}







//初始化陀螺仪、加速度计状态和协方差矩阵、为后续卡尔曼滤波做准备
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

//采集一帧 MPU6050 数据、并进行卡尔曼滤波、提取干净的动作特征
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

  input[i * input_dim] = Oy*9.8;
  input[i * input_dim + 1] = Oz*9.8;

  delay(1000 / freq); // 短暂延迟，避免过高的循环频率
}



//
int processGesture(float* output, float* confidence_out) {
//  Serial.println("模型输出:");
  int max_index = 0;
  float max_value = output[0];

  for (int i = 0; i < num_classes; i++) {
//    Serial.print("output[");
//    Serial.print(i);
//    Serial.print("] = ");
//    Serial.println(output[i]);
    if (output[i] >= max_value) {
      max_value = output[i];
      max_index = i;
    }
  }

//  Serial.print("最大置信度: ");
//  Serial.println(max_value);
//  Serial.println(max_index);
  *confidence_out = max_value; // 传出置信度

  return max_index; // 返回识别到的动作编号
}

void executeGesture(int index) {
  if (index == 0) {
//    Serial.println("执行 0Cricle 对应动作");

    //开关1闭合0.6s
    digitalWrite(IO_PIN2, HIGH);
    delay(400);
    digitalWrite(IO_PIN2, LOW);
    delay(10);

  } else if (index == 1) {
//    Serial.println("执行 1Horn 对应动作");

    //开关2闭合0.6s
    digitalWrite(IO_PIN4, HIGH);
    delay(400);
    digitalWrite(IO_PIN4, LOW);
    delay(10);


  }else if (index == 2) {
//    Serial.println("执行 2 对应动作");

    //开关3闭合0.6s
    digitalWrite(IO_PIN5, HIGH);
    delay(400);
    digitalWrite(IO_PIN5, LOW);
    delay(10);


  }else if (index == 3) {
//    Serial.println("执行 3 对应动作");

    //开关4闭合0.6s
    digitalWrite(IO_PIN6, HIGH);
    delay(400);
    digitalWrite(IO_PIN6, LOW);
    delay(10);


  }else if (index == 4) {
//    Serial.println("执行 4 对应动作");

//    switchState = !switchState;  // 每次取反，切换状态

//    if (switchState) {
//        digitalWrite(Led_PIN0, HIGH);// 开
//    } else {
//        digitalWrite(Led_PIN0, LOW);// 关
//    }

     //常亮大延迟后关闭
     digitalWrite(Led_PIN0, HIGH);// 开
     delay(10000);  //10s
     digitalWrite(Led_PIN0, LOW);// 关




  }else if (index == 5) {
//    Serial.println("执行 5 对应动作");

    


  }
}
