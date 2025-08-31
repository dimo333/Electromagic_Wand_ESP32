#include <Arduino.h>

#define BUTTON_PIN 7      // 定义按钮的引脚号
bool buttonPressed = false; // 按钮状态标志
#define DEBOUNCE_TIME 50  // 消抖时间（毫秒）
#define LONG_PRESS_TIME 1000 // 长按时间阈值（毫秒）


const int irReceiverPin = 6; // 红外接收器的OUT引脚连接到Arduino的数字引脚2
volatile unsigned long lastChangeTime = 0; // 上次电平变化的时间，需要声明为volatile
volatile unsigned long pulseWidth; // 脉冲宽度，需要声明为volatile

#define IR_BUFFER_SIZE 800 // 定义缓冲区大小
volatile unsigned long irBuffer[IR_BUFFER_SIZE]; // 存储脉冲宽度的数组
volatile int irIndex = 0; // 数组索引
volatile enum { EMPTY, RECORDING, READY } state = EMPTY; // 状态机状态

const int frequency = 38000;  // 设置PWM频率为38kHz
const int ledChannel = 0; // 设置通道为0
const int resolution = 8; // 设置分辨率为8位
const int IREMITTERPIN = 4;    // 定义红外发射器的引脚号


bool IRNOW = false; // 红外状态标志，false表示接收，true表示发送
bool IRGET = false; // 红外接收状态标志，false表示不接收，true表示接收

void setup() {
  Serial.begin(115200); // 启动串行通信
  
  pinMode(irReceiverPin, INPUT_PULLUP); // 设置引脚为输入模式并启用内部上拉电阻
  pinMode(BUTTON_PIN, INPUT_PULLUP); // 设置按钮引脚为输入并启用内部上拉



  // 配置LED引脚为输出模式
  pinMode(IREMITTERPIN, OUTPUT);

  // 配置控制器的通道
  ledcSetup(ledChannel, frequency, resolution);

  // 将GPIO与控制器通道关联
  ledcAttachPin(IREMITTERPIN, ledChannel);

  attachInterrupt(digitalPinToInterrupt(irReceiverPin), handleInterrupt, CHANGE); // 设置中断
  
}

void loop() {
  // 检测按键是否被按下
  if (digitalRead(BUTTON_PIN) == LOW) {
    if(IRNOW == false){
      IRGET = true;
      IRNOW = true;
    }else{
      irsend();
    }
    
    

    // 等待按键释放，避免重复触发
    while (digitalRead(BUTTON_PIN) == LOW);
  }
}


void irsend() {
  // 去使能中断
  detachInterrupt(digitalPinToInterrupt(irReceiverPin));
  
  Serial.print("开始发送");
  // 按键被按下，输出数组中存储的信息
  for (int i = 0; i < irIndex; i++) {
    if (i % 2 == 0) {
      // 偶数，输出高电平
      ledcWrite(ledChannel, 128);
    } else {
      // 奇数，输出低电平
      ledcWrite(ledChannel, 0);
    }
    delayMicroseconds(irBuffer[i]);
  }
  // 输出完毕后，将红外发射器引脚设置为低电平
  ledcWrite(ledChannel, 0);

  //打印
    for (int i = 0; i < irIndex; i++) {
        Serial.print(i);
        Serial.print("-");
        Serial.print(irBuffer[i]);
        Serial.println(); 
        Serial.print(" ");
      }

  // 任务执行完毕，重新使能中断
  attachInterrupt(digitalPinToInterrupt(irReceiverPin), handleInterrupt, CHANGE);
}





// 中断服务例程
void handleInterrupt() {//////////////////////////////////////////////////////////////////
  static bool firstChange = true; // 标记是否是第一次电平变化
  unsigned long currentTime = micros(); // 获取当前时间（微秒）

  if(IRGET){
    if (firstChange) {
    // 如果是第一次电平变化，从EMPTY状态转换为RECORDING状态
    state = RECORDING;
    firstChange = false; // 更新标记，后续不再视为第一次变化
    // 重置计时器
    lastChangeTime = currentTime;
    // 不记录第一次电平变化的时间
  } else {
    // 计算脉冲宽度
    pulseWidth = currentTime - lastChangeTime;
    
    // 检查脉冲宽度是否超过最大值
    if (pulseWidth > 50000) {
      // 如果脉冲宽度超过50000us，改变状态为READY
//      state = READY;
      Serial.print("超时——————————————————————");
      
    } else if (pulseWidth > 400){
      //Serial.print("除杂——————————————————————");
      // 只有在RECORDING状态下才记录脉冲宽度
      if (state == RECORDING) {
        if (irIndex < IR_BUFFER_SIZE) {
//          Serial.print("currentTime");
          irBuffer[irIndex++] = pulseWidth;
        } else {
          // 如果数组已满，改变状态为READY
          state = READY;
          Serial.print("已满——————————————————————");
        }
      }
    }
    lastChangeTime = currentTime; // 更新上次变化时间
  }
  }
  
}
