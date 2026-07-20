<div align="center">

# 📥 Get_Mpu6050_Data · Arduino 版

[![Arduino](https://img.shields.io/badge/Arduino-00979D?logo=arduino&logoColor=white)](https://www.arduino.cc/)

<sub><a href="../README.md">⬅ 返回 Get_Mpu6050_Data</a> · <a href="../espidf/README.md">切换到 ESP-IDF 版 ⚙️</a></sub>

</div>

---

采集 MPU6050 数据并经**卡尔曼滤波 + 去重力**处理后，把每次手势的
全局坐标加速度序列从**串口**打印出来，作为训练模型的原始数据。

## 🔧 使用方法

1. 确认 MPU6050 的 **SCL / SDA** 与 ESP 单片机正确连接
   （引脚不清楚见仓库根目录 [`docs/others/`](../../../docs/others) 里的 ESP 引脚图）。
2. 在 `Get_Mpu6050_Data.ino` 里按需修改：
   - `Wire.begin(sda, scl)` —— 改成你板子的 I2C 引脚（ESP32-C3 常用 `8, 9`）；
   - `buttonPin` —— 采集触发按键引脚；
   - `freq` / `second` —— 采样率与时长（默认 100Hz × 2s）。
3. 上传后打开**串口监视器**，波特率 `115200`。
4. 按下并松开按键采集一次手势，串口会输出一行逗号分隔的数据。
5. 把数据交给 [`../../Tensorflow_model_train`](../../Tensorflow_model_train)（`serial_data.py`）自动收集到 `data_x/y.csv`。

## ❗ 注意：记住你用的是哪两个轴

代码里打印的两个轴（示例为 `Ox, Oz`）**必须和最终魔杖固件
[`../../wand_example/arduino`](../../wand_example/arduino) 喂给模型的轴一致**，否则识别会错乱。

```cpp
Serial.print(Ox);   // x 轴
Serial.print(",");
Serial.print(Oz);   // z 轴
```
