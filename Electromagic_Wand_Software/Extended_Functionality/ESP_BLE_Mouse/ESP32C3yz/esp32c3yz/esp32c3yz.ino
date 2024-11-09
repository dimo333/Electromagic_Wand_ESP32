#include <Wire.h>
#include <BleMouse.h>

// MPU6050 I2C 地址
#define MPU6050_ADDR 0x68

// MPU6050寄存器定义
#define PWR_MGMT_1 0x6B
#define GYRO_YOUT_H 0x45  // 修改为读取Y轴的角速度高字节寄存器
#define GYRO_ZOUT_H 0x47
#define GYRO_CONFIG 0x1B  // Gyroscope配置寄存器

// 选择灵敏度（单位：°/s）
#define GYRO_SCALE 250  // 灵敏度：250°/s, 500°/s, 1000°/s, or 2000°/s

// 每个单位对应的角速度换算系数
float gyro_scale_factor;

int16_t gyroY, gyroZ;  // 将gyroX改为gyroY

// 滤波器参数
float alpha = 0.2;  // 滤波系数，取值范围为0~1，值越小，滤波效果越明显
float prevGyroY = 0;  // 将prevGyroX改为prevGyroY
float prevGyroZ = 0;

// 相对位移参数
float armLength = 80.0; // 臂长cm (现在保持为厘米单位) 粗略理解为灵敏度
float displacementY = 0; // 将displacementX改为displacementY
float displacementZ = 0; // Z轴累计位移

// 用于存储采集到的误差值（偏移）
float biasY = 0; // Y轴误差
float biasZ = 0; // Z轴误差

// 用于控制采集数据的计数器
int sampleCount = 0;
const int sampleSize = 50; // 采集50个数据点来计算平均值

// 定时器变量
unsigned long lastResetTime = 0;  // 上一次归零时间
const unsigned long resetInterval = 5;  // 每100毫秒（0.1秒）归零一次

// 初始化BLE鼠标
BleMouse bleMouse;

// 初始化MPU6050
void setup() {
  // 启动串口通信
  Serial.begin(115200);

  // 启动I2C通信
  Wire.begin(8,9); // ESP32的SDA和SCL引脚
  
  // 初始化MPU6050
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(PWR_MGMT_1);  // 选择电源管理寄存器
  Wire.write(0);           // 写入0以唤醒传感器
  Wire.endTransmission(true);
  delay(100);  // 等待传感器初始化
  
  // 配置陀螺仪灵敏度
  setGyroSensitivity(GYRO_SCALE);

  // 初始化蓝牙鼠标
  bleMouse.begin();

  // 打印启动状态
  Serial.println("Starting data collection...");
  delay(1000);
}

// 设置陀螺仪灵敏度
void setGyroSensitivity(int scale) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(GYRO_CONFIG);  // 选择陀螺仪配置寄存器

  switch (scale) {
    case 250:
      Wire.write(0x00);  // +/- 250°/s
      gyro_scale_factor = 1.0 / 131.0;
      break;
    case 500:
      Wire.write(0x08);  // +/- 500°/s
      gyro_scale_factor = 1.0 / 65.0;
      break;
    case 1000:
      Wire.write(0x10);  // +/- 1000°/s
      gyro_scale_factor = 1.0 / 32.8;
      break;
    case 2000:
      Wire.write(0x18);  // +/- 2000°/s
      gyro_scale_factor = 1.0 / 16.4;
      break;
    default:
      Wire.write(0x00);  // 默认设置为 250°/s
      gyro_scale_factor = 1.0 / 131.0;
  }

  Wire.endTransmission(true);
}

// 读取角速度数据
void readGyroData() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(GYRO_YOUT_H);  // 修改为读取Y轴角速度
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 2, true);
  gyroZ = (Wire.read() << 8 | Wire.read());  // 读取Z轴角速度
  
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(GYRO_ZOUT_H);  // Z轴角速度高字节寄存器地址
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 2, true);
  gyroY = (Wire.read() << 8 | Wire.read());  // 修改为读取Y轴角速度
}

// 低通滤波器
float lowPassFilter(float newValue, float prevValue, float alpha) {
  return alpha * newValue + (1 - alpha) * prevValue;
}

// 计算并设置误差修正量
void calculateBias() {
  float sumY = 0, sumZ = 0;
  for (int i = 0; i < sampleSize; i++) {
    readGyroData();

    // 进行角速度转换（单位：°/s）
    float gyroYDeg = gyroY * gyro_scale_factor;
    float gyroZDeg = gyroZ * gyro_scale_factor;

    sumY += gyroYDeg;
    sumZ += gyroZDeg;

    delay(10);  // 等待一段时间采集下一组数据
  }

  // 计算平均值
  biasY = sumY / sampleSize;
  biasZ = sumZ / sampleSize;

  // 打印计算的偏差信息
//  Serial.print("Bias Y: ");
//  Serial.print(biasY);
//  Serial.print("°/s | Bias Z: ");
//  Serial.println(biasZ);
}

// 主循环
void loop() {
  // 获取当前时间
  unsigned long currentMillis = millis();

  // 每过0.1秒，归零位移
  if (currentMillis - lastResetTime >= resetInterval) {
    displacementY = 0;
    displacementZ = 0;
    lastResetTime = currentMillis;  // 更新归零时间
  }

  // 获取原始角速度数据
  readGyroData();

  // 进行角速度转换（单位：°/s）
  float gyroYDeg = gyroY * gyro_scale_factor;
  float gyroZDeg = gyroZ * gyro_scale_factor;

  // 计算角速度差值并修正
  gyroYDeg -= biasY;  // 减去Y轴的误差修正量
  gyroZDeg -= biasZ;  // 减去Z轴的误差修正量

  // 如果角速度差值小于2°/s，则视为0
  float threshold = 2.0;  // 设置角速度阈值为2°/s
  if (abs(gyroYDeg) < threshold) {
    gyroYDeg = 0;
  }
  if (abs(gyroZDeg) < threshold) {
    gyroZDeg = 0;
  }

  // 计算角速度变化差值
  float deltaGyroY = gyroYDeg - prevGyroY;
  float deltaGyroZ = gyroZDeg - prevGyroZ;

  // 如果角速度差值小于2°/s，则忽略
  if (abs(deltaGyroY) < threshold) {
    deltaGyroY = 0;
  }
  if (abs(deltaGyroZ) < threshold) {
    deltaGyroZ = 0;
  }

  // 应用低通滤波器平滑数据
  float filteredGyroY = lowPassFilter(gyroYDeg, prevGyroY, alpha);
  float filteredGyroZ = lowPassFilter(gyroZDeg, prevGyroZ, alpha);

  // 累加位移（Y轴和Z轴），再缩小500倍
  float displacementYChange = armLength * filteredGyroY / 200.0;  // Y轴位移变化, 缩小500倍
  float displacementZChange = armLength * filteredGyroZ / 200.0;  // Z轴位移变化, 缩小500倍

  // 如果位移变化小于2cm，则忽略
  if (abs(displacementYChange) < 2.0) {
    displacementYChange = 0;
  }
  if (abs(displacementZChange) < 2.0) {
    displacementZChange = 0;
  }

  // 更新累计位移
  displacementY += displacementYChange;
  displacementZ += displacementZChange;

  // 限制位移的最大值，避免鼠标跳跃过大
  int moveX = constrain(-displacementY, -100, 100);  // 修改：displacementZ 控制左右 (moveX)
  int moveY = constrain(-displacementZ, -100, 100);  // 修改：displacementY 控制上下 (moveY)

  // 发送鼠标移动指令
  bleMouse.move(moveX, moveY, 0);

  // 更新前一次角速度数据
  prevGyroY = filteredGyroY;
  prevGyroZ = filteredGyroZ;

  // 打印调试信息
//  Serial.print("Gyro Y: ");
//  Serial.print(gyroYDeg);
//  Serial.print("°/s | Gyro Z: ");
//  Serial.print(gyroZDeg);
//  Serial.print("°/s | Displacement Y: ");
//  Serial.print(displacementY);
//  Serial.print(" cm | Displacement Z: ");
//  Serial.println(displacementZ);

  delay(10);  // 延迟以避免过快的循环
}
