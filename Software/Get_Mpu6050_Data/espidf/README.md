<div align="center">

# 📥 Get_Mpu6050_Data · ESP-IDF 版

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF%20v5.2%2B-E7352C?logo=espressif&logoColor=white)](https://docs.espressif.com/projects/esp-idf/)

<sub><a href="../README.md">⬅ 返回 Get_Mpu6050_Data</a> · <a href="../arduino/readme.md">切换到 Arduino 版 🅰️</a></sub>

</div>

---

[`../arduino/Get_Mpu6050_Data.ino`](../arduino/Get_Mpu6050_Data.ino) 的 **ESP-IDF 原生移植版**，
逻辑完全一致：按下并松开按键，采集 `FREQ × SECOND` 个样本，经卡尔曼滤波去重力后，
把选定的两个全局坐标轴按 **CSV 一行** 打印到串口，供 [`../../Tensorflow_model_train`](../../Tensorflow_model_train) 收集训练。

复用了魔杖 ESP-IDF 固件里的 [`mpu6050`](../../wand_example/espidf/main) 驱动与
[`kalman_wand`](../../wand_example/espidf/main) 滤波模块（无需 TFLite 依赖）。

---

## ✅ 环境要求

- **ESP-IDF v5.2+**（使用新版 `driver/i2c_master.h`）。
- 目标芯片：ESP32-C3 SuperMini（其它型号改引脚即可）。

---

## 🚀 编译与烧录

```bash
cd Software/Get_Mpu6050_Data/espidf

idf.py set-target esp32c3      # 或 esp32 / esp32s3
idf.py build
idf.py -p COMx flash monitor   # COMx 换成你的串口
```

`monitor` 打开后，每按一次按键就会输出一行 CSV，例如：

```text
0.12,-0.98,0.15,-0.95, ... ,0.02,-1.01
```

一行 = 一次手势 = `FREQ×SECOND` 个样本 × 2 轴（默认 200×2 = 400 个数）。

---

## 🔌 默认接线（与魔杖固件一致）

| 功能        | GPIO | 说明                     |
|-------------|------|--------------------------|
| MPU6050 SDA | 8    | I2C 数据                 |
| MPU6050 SCL | 9    | I2C 时钟                 |
| 按键        | 4    | 接地按键，内部上拉       |

引脚、采样率、轴都在 [`main/get_data_main.c`](main/get_data_main.c) 顶部的
`USER CONFIGURATION` 区集中定义。

---

## ❗ 轴必须对齐

`get_data_main.c` 里 `AXIS_0 / AXIS_1` 决定录制哪两个轴：

- `AXIS_0=1, AXIS_1=2` → **y、z 轴**（默认，对应 0922_6 / 0908_6 模型）
- `AXIS_0=0, AXIS_1=2` → **x、z 轴**（旧 1030_5 / Arduino 采集脚本）

> 🚨 采集、训练、魔杖固件三处使用的轴与采样率必须**完全一致**，否则识别会错乱。
> 各模型参数见 [`../../models/README.md`](../../models/README.md)。

---

## 🔍 与 Arduino 版的对应关系

| Arduino (.ino)          | ESP-IDF 版                                |
|-------------------------|-------------------------------------------|
| `Wire` + `MPU6050` 库   | `mpu6050.c`（`i2c_master` 驱动）          |
| `kalman_update()`       | `wand_kalman_step()`                       |
| `resetState()`          | `wand_kalman_reset()`                      |
| `Serial.print(Ox,Oz)`   | `printf("%.2f,%.2f", ori[AXIS_0], ...)`   |
| `millis()` / `delay()`  | `esp_timer_get_time()` / `vTaskDelay()`   |
| `loop()` 按键去抖       | `loop_task()`（独立 FreeRTOS 任务）       |
