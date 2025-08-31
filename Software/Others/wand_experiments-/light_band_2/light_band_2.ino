#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 20              // 控制的LED数量
#define DATA_PIN 4               // 控制LED的引脚
#define BUTTON_PIN 7             // 按键的引脚

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

// 用于记录发射方向的变量
bool isBottomToTop = true;   // 默认从底端发射

void setup() {
  strip.begin();
  strip.show();
  pinMode(BUTTON_PIN, INPUT_PULLUP); // 设置按键引脚为输入，并启用内部上拉电阻
}

void loop() {
  static bool lastButtonState = HIGH;  // 上一次的按键状态
  bool currentButtonState = digitalRead(BUTTON_PIN);  // 当前的按键状态

  // 检测按键按下（从HIGH到LOW变化）
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    // 每次按下时，切换发射方向
    isBottomToTop = !isBottomToTop;  // 切换发射方向
    if (isBottomToTop) {
      energyGatherAndBurst(300, 10, true);  // 从底端发射
    } else {
      energyGatherAndBurst(300, 10, false); // 从顶端发射
    }
  }

  lastButtonState = currentButtonState;  // 更新按键状态
}

void energyGatherAndBurst(uint8_t gatherDelay, uint8_t burstDelay, bool fromBottom) {
  int baseLeds = 1;  // 底部亮起的LED数量（可以调整）

  // 聚集效果 - 根据发射方向选择聚集位置
  if (fromBottom) {
    // 从底端发射时，聚集效果从底部开始
    for (int brightness = 0; brightness <= 255; brightness += 5) {
      for (int i = 0; i < baseLeds; i++) {
        strip.setPixelColor(i, strip.Color(0, 200, 255));  // 设置浅蓝色，逐渐变亮
      }
      strip.show();
      delay(gatherDelay);
    }
  } else {
    // 从顶端发射时，聚集效果从顶端开始
    for (int brightness = 0; brightness <= 255; brightness += 5) {
      for (int i = strip.numPixels() - 1; i >= strip.numPixels() - baseLeds; i--) {
        strip.setPixelColor(i, strip.Color(0, 200, 255));  // 设置浅蓝色，逐渐变亮
      }
      strip.show();
      delay(gatherDelay);
    }
  }

  // 爆发效果 - LED逐一亮起
  if (fromBottom) {
    // 从底端发射
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 200, 255));  // 设置浅蓝色
      strip.show();
      delay(burstDelay);

      // 熄灭前一个LED
      if (i > 0) {
        strip.setPixelColor(i - 1, strip.Color(0, 0, 0));  // 熄灭前一个LED
      }
    }
  } else {
    // 从顶端发射
    for (int i = strip.numPixels() - 1; i >= 0; i--) {
      strip.setPixelColor(i, strip.Color(0, 200, 255));  // 设置浅蓝色
      strip.show();
      delay(burstDelay);

      // 熄灭前一个LED
      if (i < strip.numPixels() - 1) {
        strip.setPixelColor(i + 1, strip.Color(0, 0, 0));  // 熄灭前一个LED
      }
    }
  }

  // 清除灯带上的亮光
  strip.clear();
  strip.show();
  delay(500);  // 增加一个小的延迟以区分效果循环
}
