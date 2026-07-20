<div align="center">

# 🪄 wand_example · 魔杖主固件

**按键触发 → 采集手势 → 卡尔曼滤波 → TFLite Micro 推理 → 执行动作**

[![Arduino](https://img.shields.io/badge/Arduino-00979D?logo=arduino&logoColor=white)](arduino)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF%20v5.2%2B-E7352C?logo=espressif&logoColor=white)](espidf)
[![TensorFlow Lite](https://img.shields.io/badge/TFLite%20Micro-FF6F00?logo=tensorflow&logoColor=white)](https://www.tensorflow.org/lite/microcontrollers)

<sub><a href="../README.md">⬅ 返回 Software</a></sub>

</div>

---

魔杖**最终烧录**的主程序，提供 **两种框架** 实现，功能完全一致，任选其一。

## 📂 两套实现

| | <img src="https://cdn.simpleicons.org/arduino/00979D" height="15"/> Arduino | <img src="https://cdn.simpleicons.org/espressif/E7352C" height="15"/> ESP-IDF |
|---|:---:|:---:|
| 目录 | [`arduino/`](arduino) | [`espidf/`](espidf) |
| 适合人群 | 新手 / 快速上手 | 进阶 / 追求性能与工程化 |
| 依赖 | Arduino IDE + 库 | ESP-IDF v5.2+ |
| I2C 驱动 | `Wire` | 原生 `i2c_master` |
| AI 运行时 | TensorFlowLite_ESP32 | esp-tflite-micro |
| 上手文档 | [arduino/README.md](arduino/README.md) | [espidf/README.md](espidf/README.md) |

> 📌 ESP-IDF 版是对 Arduino 版的 **1:1 原生移植**，模块拆分清晰（MPU6050 驱动 / 卡尔曼滤波 / 推理主程序）。

## 🔗 配套

| 环节 | 目录 |
|------|------|
| 📥 采集训练数据 | [`../Get_Mpu6050_Data`](../Get_Mpu6050_Data) |
| 🐍 训练并导出模型 | [`../Tensorflow_model_train`](../Tensorflow_model_train) |
| 🧠 现成模型 | [`../models`](../models) |
| 🔌 接线 / 引脚图 | [`../../docs/others`](../../docs/others) |

> ⚠️ **轴要对齐**：固件喂给模型的传感器轴（如 y+z）必须与采集 / 训练时一致，否则识别会错乱。
> 各模型的轴与采样率见 [`../models/README.md`](../models/README.md)。
