<div align="center">

# 🐍 Tensorflow_model_train · 模型训练

**PC 端采集、训练与可视化手势数据的 Python 脚本**

![Python](https://img.shields.io/badge/Python-3776AB?logo=python&logoColor=white)
![TensorFlow](https://img.shields.io/badge/TensorFlow-FF6F00?logo=tensorflow&logoColor=white)

来源：dimo333 · <sub><a href="../README.md">⬅ 返回 Software</a></sub>

</div>

---

## 🚀 使用方法

1. **先认真看图文教程**中关于环境安装的部分，装好 Python 与依赖。
2. 用 [`serial_data.py`](serial_data.py) 从串口收集手势数据到 `data_x/y.csv`。
3. 用 [`train.py`](train.py) 训练，得到 `.tflite` 模型。
4. `.tflite` 需转成 `.h` 才能写进 ESP32，在 Git Bash / Linux 下执行：

```bash
xxd -i model.tflite > model.h
```

5. 把 `model.h` 拷进魔杖固件 [`../wand_example`](../wand_example)。

## 🧰 代码与工具

| 脚本 | 作用 | 来源 |
|------|------|------|
| [`serial_data.py`](serial_data.py) | **主体**：收集数据并写入 `data_x/y.csv`，为 `train.py` 做准备 | dimo333 |
| [`train.py`](train.py) | **主体**：训练手势模型（需转格式后供固件使用） | dimo333 |
| [`serial_data_show.py`](serial_data_show.py) | 收集数据时直观查看手势波形质量 | dimo333 |
| [`view.py`](view.py) | 查看 `show.csv` 中数据的波形质量 | 群友「雪云」 |
| [`show2.py`](show2.py) | 受「雪云」启发改写的波形查看工具（读 `show.csv`） | dimo333 |
| [`Dimo333_Data_Management_Tool.py`](Dimo333_Data_Management_Tool.py) | 手势数据质量管理工具（读 `show.csv`） | dimo333 |

> `serial_data.py` 与 `train.py` 构成训练主体，其余为辅助工具。

## ⚠️ 注意事项

- 代码版本较旧，部分参数不一定最优，请自行修改。
- 手势训练是魔杖核心之一，**数据录入需要极大的耐心与定力**——用群友的话说，
  “这是一场魔杖选择主人巫师的过程”，请量力而行。
- 采样率已由 **64Hz×2s 升级到 100Hz×2s**，除主体外的三个辅助脚本均围绕新采样率开发，
  使用时注意数据格式的兼容性（对应 `train.py` 里的 `timesteps`：64Hz→128，100Hz→200）。
