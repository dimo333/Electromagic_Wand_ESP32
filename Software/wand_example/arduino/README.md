<div align="center">

# 🪄 wand_example · Arduino 版

[![Arduino](https://img.shields.io/badge/Arduino-00979D?logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![TensorFlow Lite](https://img.shields.io/badge/TFLite%20Micro-FF6F00?logo=tensorflow&logoColor=white)](https://www.tensorflow.org/lite/microcontrollers)

<sub><a href="../README.md">⬅ 返回 wand_example</a> · <a href="../espidf/README.md">切换到 ESP-IDF 版 ⚙️</a></sub>

</div>

---

魔杖**最终烧录**的主固件：按下并松开按键采集一段手势，经卡尔曼滤波与去重力后
喂入 TensorFlow Lite Micro 模型，识别手势并执行对应动作（示例：类别 0 闪灯）。

## 🔧 使用步骤

1. 用 [`../../Get_Mpu6050_Data/arduino`](../../Get_Mpu6050_Data/arduino) 采集数据，用
   [`../../Tensorflow_model_train`](../../Tensorflow_model_train) 训练并导出 `model.h`。
2. 把 `model.h` 放到本目录（`#include "model.h"`）。
3. 在 `wand_example.ino` 顶部按需修改：
   - `num_classes` —— 手势数量；
   - `input_dim` —— 每个采样点的轴数（参考为 2）；
   - `buttonPin` / `ledPin` —— 按键与 LED 引脚；
   - `freq` / `second` —— 采样率与时长（必须与训练时一致）。
4. 选好开发板与串口，上传。

## ❗ 最重要的一点：轴必须对齐

魔杖识别是否准确，取决于**采集、训练、固件三处使用的传感器轴是否一致**。

在 [`Get_Mpu6050_Data`](../../Get_Mpu6050_Data/arduino) 里，串口打印的是哪两个轴：

```cpp
Serial.print(Ox);   // ← 这里用的是 x
Serial.print(",");
Serial.print(Oz);   // ← 和 z 轴
```

那么本固件里喂给模型的输入就必须是**同样的轴**：

```cpp
input[i * input_dim]     = Ox;   // 保持与上面一致
input[i * input_dim + 1] = Oz;
```

> 🚨 一旦两处的轴不一致（比如一个 x+z、一个 y+z），识别结果会完全错乱。
> 仓库自带模型的轴与采样率见 [`../../models/README.md`](../../models/README.md)。

## 🔁 想要 ESP-IDF 版？

功能完全一致的原生 ESP-IDF 移植见 [`../espidf`](../espidf)。
