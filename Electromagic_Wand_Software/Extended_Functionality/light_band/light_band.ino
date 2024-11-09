#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 68///////////////////////////////////////////////////这里填写你想要控制灯的数量
#define DATA_PIN 4///////////////////////////////////////////////////这里填写你想要控制灯的引脚
#define BUTTON_PIN 7 //////////////////////////////////////////////////按键的引脚

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show();
  pinMode(BUTTON_PIN, INPUT_PULLUP); // 设置按键引脚为输入，并启用内部上拉电阻
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    energyGatherAndBurst(30, 10); ///////////////////////////////////// 调整这些值来控制效果速度
  }
}

void energyGatherAndBurst(uint8_t gatherDelay, uint8_t burstDelay) {
  int baseLeds = 1; ///////////////////////////////////灯带亮起时底部亮起的LED数量
  // 聚集效果 - 底部LED逐渐变亮
  for (int brightness = 0; brightness <= 255; brightness += 5) {
    for (int i = 0; i < baseLeds; i++) {
      strip.setPixelColor(i, strip.Color(brightness, brightness, brightness));
    }
    strip.show();
    delay(gatherDelay);
  }
  // 爆发效果 - LED从底部快速移动到顶部
  strip.clear();
  for (int i = 0; i < baseLeds; i++) {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
  }
  strip.show();
  delay(burstDelay);
  for (int i = 1; i <= strip.numPixels() - baseLeds; i++) {
    strip.clear();
    for (int j = 0; j < baseLeds; j++) {
      strip.setPixelColor(i + j, strip.Color(255, 255, 255));
    }
    strip.show();
    delay(burstDelay);
  }
  // 清除顶部亮光
  strip.clear();
  strip.show();
  delay(500); // 增加一个小的延迟以区分效果循环
}
