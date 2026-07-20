<div align="center">

# 🪄 wand_example · ESP-IDF 版

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF%20v5.2%2B-E7352C?logo=espressif&logoColor=white)](https://docs.espressif.com/projects/esp-idf/)
[![esp-tflite-micro](https://img.shields.io/badge/esp--tflite--micro-FF6F00?logo=tensorflow&logoColor=white)](https://github.com/espressif/esp-tflite-micro)

<sub><a href="../README.md">⬅ 返回 wand_example</a> · <a href="../arduino/README.md">切换到 Arduino 版 🅰️</a></sub>

</div>

---

这是 [`Software/wand_example/arduino/wand_example.ino`](../arduino/wand_example.ino) 的 **ESP-IDF 原生移植版**，
功能与 Arduino 版完全一致：

1. 按下并松开按键 → 采集 `FREQ × SECOND` 个样本的手势数据；
2. 用卡尔曼滤波把 MPU6050 的加速度/角速度融合成姿态，去除重力并旋转到全局坐标系；
3. 把 `(Ox, Oy, Oz)` 中选定的两个轴喂给 TensorFlow Lite Micro 模型；
4. 取 arg-max 得到手势类别并触发动作（示例：类别 0 闪灯）。

---

## ✅ 环境要求

- **ESP-IDF v5.2 及以上**（本移植用了新的 `driver/i2c_master.h` I2C 驱动）。
- 目标芯片：ESP32-C3 SuperMini（也支持 ESP32 / C3 / S3 等，改一下引脚即可）。
- 依赖组件 `esp-tflite-micro` 会在首次 `idf.py build` 时由组件管理器自动下载
  （见 [`main/idf_component.yml`](main/idf_component.yml)）。

> ⚠️ 你机器上的 `IDF_PATH` 目前指向 `esp-idf-v5.5.3`，但磁盘上实际只找到
> `esp-idf-v4.4.2`。**4.4.2 无法编译本工程**（没有新版 I2C 驱动，TFLite API 也不同）。
> 请先安装/激活 5.2+ 的 ESP-IDF，再执行下面的命令。

---

## 🚀 编译与烧录

```bash
cd Software/wand_example/espidf

idf.py set-target esp32c3      # 或 esp32 / esp32s3
idf.py build
idf.py -p COMx flash monitor   # COMx 换成你的串口
```

---

## 🔌 默认接线（ESP32-C3 SuperMini）

| 功能        | GPIO | 说明                                   |
|-------------|------|----------------------------------------|
| MPU6050 SDA | 8    | I2C 数据                               |
| MPU6050 SCL | 9    | I2C 时钟                               |
| 按键        | 4    | 接地按键，内部上拉（INPUT_PULLUP）     |
| LED         | 5    | 示例输出                               |

引脚都在 [`main/wand_main.cpp`](main/wand_main.cpp) 顶部的
`USER CONFIGURATION` 区集中定义，按需修改。

---

## 🧠 换成你自己的模型（重要）

1. 用训练脚本导出 `model.h`，覆盖 [`main/model.h`](main/model.h)
   （里面必须有 `unsigned char model_tflite[]`）。
2. 在 [`main/wand_main.cpp`](main/wand_main.cpp) 顶部同步修改：
   - `NUM_CLASSES` —— 手势数量；
   - `INPUT_DIM` —— 每个采样点的轴数（参考模型为 2）；
   - `AXIS_0 / AXIS_1` —— **喂给模型的是哪两个轴**。
     `wand_kalman_step()` 输出 `{Ox, Oy, Oz}` 对应下标 `0,1,2`。
     - 本仓库自带的 [`Software/models/0922_6`](../../models/0922_6) 模型：**y、z 轴、100Hz×2s、6 类、识别率约 95%** → `AXIS_0=1, AXIS_1=2`（当前默认）。
     - 若换用 `1030_5` 模型（x、z 轴、64Hz×2s、5 类）→ 改成 `AXIS_0=0, AXIS_1=2`，并把 `SECOND` 保持 2、`FREQ` 改成 64。
   - `FREQ / SECOND` —— 采样率与时长，**必须与模型训练时的时间步一致**
     （0922_6/0908_6 = 100Hz×2s = 200 步；1030_5 = 64Hz×2s = 128 步）。

   > ❗ 轴与采样率必须和你**录数据/训练时完全一致**，否则识别会乱。
   > 各模型的轴/采样率见 [`Software/models/README.md`](../../models/README.md)。
3. 如果模型用到的算子和参考模型不同，需在 `setup()` 里增删
   `resolver.AddXxx()`，并把 `MicroMutableOpResolver<7>` 的数字改成算子个数。

---

## 📂 文件结构

```text
wand_example/espidf/
├── CMakeLists.txt          # 顶层工程
├── sdkconfig.defaults      # FreeRTOS 1kHz tick、性能优化等
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml   # 声明 esp-tflite-micro 依赖
│   ├── wand_main.cpp       # app_main：按键/推理/触发（对应 .ino 的 loop）
│   ├── mpu6050.h/.c        # MPU6050 I2C 驱动（i2c_master API）
│   ├── kalman_wand.h/.c    # 卡尔曼姿态滤波（对应 .ino 的 get_kalman_mpu_data）
│   └── model.h             # TFLite 模型（默认 = models/0922_6）
└── README.md
```

---

## 🔍 与 Arduino 版的对应关系

| Arduino (.ino)              | ESP-IDF 版                                   |
|-----------------------------|----------------------------------------------|
| `Wire` + `MPU6050` 库       | `mpu6050.c`（`i2c_master` 驱动）             |
| `get_kalman_mpu_data()`     | `wand_kalman_step()`                          |
| `resetState()`              | `wand_kalman_reset()`                         |
| `millis()` / `delay()`      | `esp_timer_get_time()` / `vTaskDelay()`      |
| `AllOpsResolver`            | `MicroMutableOpResolver<7>`（只注册用到的算子）|
| `loop()` 按键去抖           | `loop_task()`（独立 FreeRTOS 任务）          |
