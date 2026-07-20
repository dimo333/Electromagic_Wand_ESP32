<div align="center">

# 💻 Software

**魔杖的全部软件** — 单片机固件、手势模型与 PC 端训练脚本

[![Arduino](https://img.shields.io/badge/Arduino-00979D?logo=arduino&logoColor=white)](wand_example/arduino)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-E7352C?logo=espressif&logoColor=white)](wand_example/espidf)
[![TensorFlow Lite](https://img.shields.io/badge/TFLite%20Micro-FF6F00?logo=tensorflow&logoColor=white)](wand_example)
[![Python](https://img.shields.io/badge/Python-3776AB?logo=python&logoColor=white)](Tensorflow_model_train)

<sub><a href="../README.md">⬅ 返回项目主页</a></sub>

</div>

---

组织原则：**按功能分，再按框架分** —— 每个功能目录里再放 `arduino/` 与 `espidf/` 两种实现。

```text
Software/
├── wand_example/           🪄 魔杖主固件（推理 + 触发动作）
│   ├── arduino/                Arduino 版
│   └── espidf/                 ESP-IDF 版（Arduino 版的原生移植）
├── Get_Mpu6050_Data/       📥 数据采集（串口输出，供训练用）
│   ├── arduino/                Arduino 版
│   └── espidf/                 ESP-IDF 版
├── Others/                 🧪 红外 / BLE 键鼠 / WiFi-MQTT / 灯带等实验
├── models/                 🧠 训练好的手势模型（.tflite / model.h，各版本共用）
└── Tensorflow_model_train/ 🐍 PC 端训练与数据处理脚本（Python）
```

---

## 🧭 我该从哪开始？

| 你的目标 | 去这里 |
|----------|--------|
| 🚀 快速点亮一根魔杖（新手） | [`wand_example/arduino`](wand_example/arduino) |
| ⚙️ 用原生 ESP-IDF 工程化开发 | [`wand_example/espidf`](wand_example/espidf) |
| 📥 采集手势数据训练新法术 | [`Get_Mpu6050_Data`](Get_Mpu6050_Data) → [`Tensorflow_model_train`](Tensorflow_model_train) |
| 🧠 直接使用现成模型 | [`models`](models) |
| 🧪 玩红外 / 蓝牙键鼠 / MQTT 等扩展 | [`Others`](Others) |

---

## 🔗 完整流程

```text
   ①采集数据              ②训练模型                ③转换格式          ④烧录固件
Get_Mpu6050_Data  ──▶  Tensorflow_model_train  ──▶  xxd -i  ──▶  model.h  ──▶  wand_example
   (Arduino)               (PC / Python)                                     (Arduino / ESP-IDF)
```

> ⚠️ **务必对齐"轴"和"采样率"**
> 采集、训练、固件三处使用的传感器轴（如 y+z）与采样率（如 100Hz×2s）必须**完全一致**，
> 否则识别会错乱。各模型的参数见 [`models/README.md`](models/README.md)。
