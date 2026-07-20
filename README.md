<div align="center">

# 🧙‍♂️ Electromagic Wand ESP32

### 基于 ESP32 + MPU6050 的手势识别电子魔杖

**挥动魔杖，念动咒语，让现实世界响应你的每一个手势。**

<br/>

<!-- Tech-stack badges (real brand logos) -->
[![ESP32-C3](https://img.shields.io/badge/ESP32--C3-E7352C?logo=espressif&logoColor=white)](https://www.espressif.com/)
[![Arduino](https://img.shields.io/badge/Arduino-00979D?logo=arduino&logoColor=white)](Software/wand_example/arduino)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-E7352C?logo=espressif&logoColor=white)](Software/wand_example/espidf)
[![TensorFlow Lite](https://img.shields.io/badge/TensorFlow%20Lite%20Micro-FF6F00?logo=tensorflow&logoColor=white)](https://www.tensorflow.org/lite/microcontrollers)
[![立创EDA](https://img.shields.io/badge/立创EDA-1476FF?logo=easyeda&logoColor=white)](https://oshwhub.com/6dimo9/electromagic_wand_esp32)
[![License](https://img.shields.io/badge/License-GPLv3-A42E2B?logo=gnu&logoColor=white)](LICENSE)

<!-- Portals (real brand logos) -->
[![GitHub](https://img.shields.io/badge/GitHub-181717?logo=github&logoColor=white)](https://github.com/dimo333/Electromagic_Wand_ESP32)
[![Gitee](https://img.shields.io/badge/Gitee-C71D23?logo=gitee&logoColor=white)](https://gitee.com/dimo666/Electromagic_Wand_ESP32/tree/main)
[![PCB 立创开源](https://img.shields.io/badge/PCB_立创开源-1476FF?logo=easyeda&logoColor=white)](https://oshwhub.com/6dimo9/electromagic_wand_esp32)
[![Bilibili](https://img.shields.io/badge/B站演示-00A1D6?logo=bilibili&logoColor=white)](https://www.bilibili.com/video/BV1NtwreXEzg)
[![Stars](https://img.shields.io/github/stars/dimo333/Electromagic_Wand_ESP32?style=social)](https://github.com/dimo333/Electromagic_Wand_ESP32)

<br/>

<img src="docs/others/wand-pcb-hero.jpg" alt="魔杖硬件" width="70%"/>

</div>

---

## 📖 项目简介

这是一个基于 **ESP32-C3 + MPU6050** 打造的 **手势识别电子魔杖**。它在指尖运行一个轻量级
神经网络（TensorFlow Lite Micro），实时识别你在空中挥出的"咒语"，并据此触发红外遥控、
模拟键鼠、智能家居联动等各种动作。

> 🪄 你可以直接复刻本项目，也可以用提供的训练代码"**训练属于你自己的法术**"。
> 欢迎交流、学习与二次创作！

<div align="center">

| 🎯 手势识别 | 🧠 端侧 AI | 🛠 完全开源 | 💰 低成本 |
|:---:|:---:|:---:|:---:|
| 卡尔曼滤波姿态解算 | TFLite Micro 推理 | 代码 / PCB / 3D 外壳 | 模块方案约 15 元 |

</div>

---

## ✨ 魔杖能做什么？

魔杖以手势识别为核心，可扩展出丰富的交互能力：

| 功能 | 说明 |
|------|------|
| 📡 **红外遥控** | 复制并发送红外信号，一杖遥控家中电器 |
| 🖱 **模拟鼠标** | 化身"飞鼠"，隔空操作光标 |
| ⌨️ **模拟键盘** | 手势触发快捷键与文本输入 |
| 📶 **蓝牙 / WiFi** | 接入 HomeAssistant / 米家，联动智能家居 |
| 💡 **自定义动作** | 每个手势对应一段你自己的代码逻辑 |

---

## 🖐 内置手势示例

<div align="center">

<table>
  <tr>
    <td align="center"><img src="docs/gestures/Circle.jpg" width="120"/><br/>Circle</td>
    <td align="center"><img src="docs/gestures/Triangle.jpg" width="120"/><br/>Triangle</td>
    <td align="center"><img src="docs/gestures/Lignitning.jpg" width="120"/><br/>Lightning</td>
    <td align="center"><img src="docs/gestures/Wave.jpg" width="120"/><br/>Wave</td>
  </tr>
  <tr>
    <td align="center"><img src="docs/gestures/Letter_W.jpg" width="120"/><br/>Letter W</td>
    <td align="center"><img src="docs/gestures/Letter_Phi.jpg" width="120"/><br/>Letter Φ</td>
    <td align="center"><img src="docs/gestures/Horn_.jpg" width="120"/><br/>Horn</td>
    <td align="center"><img src="docs/gestures/Up_And_Down.jpg" width="120"/><br/>Up &amp; Down</td>
  </tr>
</table>

<sub>更多手势见 <a href="docs/gestures/">docs/gestures/</a> —— 你也可以设计并训练全新的手势 ✨</sub>

</div>

---

## 🔥 两套固件任你选择

本仓库同时提供 **Arduino** 与 **ESP-IDF** 两套实现，功能完全一致：

<div align="center">

| | <img src="https://cdn.simpleicons.org/arduino/00979D" height="15"/> Arduino 版 | <img src="https://cdn.simpleicons.org/espressif/E7352C" height="15"/> ESP-IDF 版 |
|---|:---:|:---:|
| 目录 | [`Software/wand_example/arduino`](Software/wand_example/arduino) | [`Software/wand_example/espidf`](Software/wand_example/espidf) |
| 适合人群 | 新手 / 快速上手 | 进阶 / 追求性能与工程化 |
| 依赖 | Arduino IDE + 库 | ESP-IDF v5.2+ |
| I2C 驱动 | `Wire` | 原生 `i2c_master` |
| AI 运行时 | TensorFlow Lite Micro | esp-tflite-micro |

</div>

> 📌 ESP-IDF 版是对 Arduino 版的 1:1 原生移植，模块拆分清晰（MPU6050 驱动 / 卡尔曼滤波 / 推理主程序），
> 详见 [ESP-IDF 版 README](Software/wand_example/espidf/README.md)。

---

## 🚀 快速开始

我已编写详细的图文教程和环境配置指南，跟着做即可复刻：

- <img src="https://cdn.simpleicons.org/github/181717/white" height="14" align="top"/> &nbsp;[教程文档 (GitHub)](https://github.com/dimo333/Electromagic_Wand_ESP32/tree/main/docs/tutorial)
- <img src="https://cdn.simpleicons.org/gitee/C71D23" height="14" align="top"/> &nbsp;[教程文档 (Gitee)](https://gitee.com/dimo666/Electromagic_Wand_ESP32/tree/main/docs/tutorial)
- <img src="https://cdn.simpleicons.org/bilibili/00A1D6" height="14" align="top"/> &nbsp;[B站视频教程（基础演示）](https://www.bilibili.com/video/BV1NtwreXEzg)

<div align="center">

**复刻路线图**

<img src="docs/Guidelines.png" alt="复刻路线" width="85%"/>

</div>

---

## 💰 硬件清单与预算

<table>
<tr>
<td width="50%" valign="top">

### 🔧 模块方案（入门推荐）

**成本约 15 元**

- ESP32-C3 SuperMini（4MB Flash）
- MPU6050 陀螺仪模块
- 按键模块（可选）

> 用杜邦线/面包板即可连接，免焊接门槛低。

</td>
<td width="50%" valign="top">

### 🛠 自制 PCB（进阶推荐）

**成本约 70 元**

- 定制 PCB 电路板（支持免费打样）
- ESP32-C3FH4 芯片、MPU6050 等元件
- 推荐平台：嘉立创 / 立创商城

> 成品更紧凑美观，接近"魔杖"手感。

</td>
</tr>
</table>

> 🔌 **接线图 / 引脚图 / 硬件资料** 见 [`docs/others`](docs/others)（含 ESP32-C3 SuperMini、WROOM、MPU6050 的接线与引脚图）。

> 🧞 **不想动手？** 加入 QQ 群 **884880944** 问问有没有大佬出售焊好的板子，或私聊作者看看有没有现成板。

---

## 📁 项目结构

```text
Electromagic_Wand_ESP32/
├── docs/          📚 项目文档、图文教程、手势与硬件图
├── Software/      💻 软件代码
│   ├── wand_example/          魔杖主固件
│   │   ├── arduino/               Arduino 版
│   │   └── espidf/                ESP-IDF 版 
│   ├── Get_Mpu6050_Data/     数据采集（用于训练）
│   │   ├── arduino/               Arduino 版
│   │   └── espidf/                ESP-IDF 版 
│   ├── Others/               红外 / BLE / WiFi 等实验代码
│   ├── models/               🧠 训练好的手势模型（各版本共用）
│   └── Tensorflow_model_train/ 🐍 PC 端训练与数据处理脚本
├── Hardware/      🔩 原理图、PCB 设计文件
├── 3D_Print/      🖨 3D 打印外壳模型
└── lib/           📦 依赖库（MPU6050 / TFLite ESP32）
```

---

## 🛠 遇到问题？

按以下顺序自救，通常都能解决：

1. 🔍 仔细检查是否遗漏了教程中的某一步。
2. 📄 查阅 [H_elp 文档](https://github.com/dimo333/Electromagic_Wand_ESP32/tree/main/docs/H_elp)。
3. 🤖 复制错误信息，用搜索引擎或 AI 工具查找答案。
4. 💬 加入 QQ 群：**884880944**。
5. 📺 B站私信作者：[dimo333](https://space.bilibili.com/)。
6. 📧 邮件联系：`dimo333mail@gmail.com` / `3180405726@qq.com`

---

## 🤝 鸣谢与参考

- [**MagicWand**](https://github.com/Songyeyaosong/MagicWand) —— 最早实现了赛博魔杖的底层功能，为本项目提供了核心参考。
- [**esp32_mouse_keyboard**](https://github.com/asterics/esp32_mouse_keyboard) —— 提供 BLE 鼠标 / 键盘模拟能力，丰富了魔杖的交互。

---

## 📜 开源许可

本项目基于 [GNU GPLv3](LICENSE) 协议开源。欢迎学习、复刻与二次创作，若你做出了有趣的改进，欢迎交流分享！

<div align="center">

<br/>

**如果这个项目帮到了你，别忘了点一个 ⭐ Star 支持一下！**

<sub>Made with 🪄 by <a href="https://github.com/dimo333">dimo333</a> and the community.</sub>

</div>
