<div align="center">

# 📥 Get_Mpu6050_Data · 数据采集

**按键触发 → 采样 → 卡尔曼滤波去重力 → 串口输出 CSV**（供训练用）

[![Arduino](https://img.shields.io/badge/Arduino-00979D?logo=arduino&logoColor=white)](arduino)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF%20v5.2%2B-E7352C?logo=espressif&logoColor=white)](espidf)

<sub><a href="../README.md">⬅ 返回 Software</a></sub>

</div>

---

采集 MPU6050 数据，经卡尔曼滤波与去重力处理后，把每次手势的全局坐标加速度序列
从**串口**按一行 CSV 打印出来，作为训练模型的原始数据。

## 📂 两套实现

| | <img src="https://cdn.simpleicons.org/arduino/00979D" height="15"/> Arduino | <img src="https://cdn.simpleicons.org/espressif/E7352C" height="15"/> ESP-IDF |
|---|:---:|:---:|
| 目录 | [`arduino/`](arduino) | [`espidf/`](espidf) |
| 依赖 | Arduino IDE + 库 | ESP-IDF v5.2+ |
| 上手文档 | [arduino/readme.md](arduino/readme.md) | [espidf/README.md](espidf/README.md) |

> ESP-IDF 版复用了魔杖固件的 `mpu6050` 驱动与 `kalman_wand` 滤波模块，逻辑与 Arduino 版一致。

## 🔗 下一步

采到的数据交给 [`../Tensorflow_model_train`](../Tensorflow_model_train)（`serial_data.py`）收集与训练，
最终导出的 `model.h` 用于魔杖固件 [`../wand_example`](../wand_example)。

> ⚠️ 采集时用的传感器轴（如 y+z）必须与固件、训练时**完全一致**。
> 接线与坐标轴见 [`../../docs/others`](../../docs/others)。
