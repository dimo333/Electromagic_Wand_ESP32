import matplotlib.pyplot as plt
import matplotlib
import numpy as np
import time
import csv

# 设置支持中文字体（推荐：SimHei 或 Microsoft YaHei）
matplotlib.rcParams['font.family'] = 'SimHei'  # 或 'Microsoft YaHei'
matplotlib.rcParams['axes.unicode_minus'] = False  # 正确显示负号
# CSV 文件路径
csv_file_path = 'E:\Dtemp\Electromagic_Wand_ESP32-main\Electromagic_Wand_ESP32-main\Electromagic_Wand_Software\Basic\Tensorflow_model_train\show.csv'

# 开启交互式模式
plt.ion()
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 6))

# 图表初始化设置
ax1.set_ylim(-2, 2)
ax1.set_ylabel('加速度 (g)')
ax1.set_title('MPU6050 数据分析图')
ax1.grid(True)
line1, = ax1.plot([], [], 'r-', label='Oy (Y轴加速度)')
ax1.legend()

ax2.set_ylim(-2, 2)
ax2.set_xlabel('采样点序号')
ax2.set_ylabel('加速度 (g)')
ax2.grid(True)
line2, = ax2.plot([], [], 'b-', label='Oz (Z轴加速度)')
ax2.legend()

# 逐行读取并绘图
with open(csv_file_path, 'r') as file:
    reader = csv.reader(file)
    for row in reader:
        if not row:
            continue
        try:
            data_list = [float(x.strip()) for x in row if x.strip()]
            oy_values = data_list[::2]
            oz_values = data_list[1::2]
            time_points = np.arange(len(oy_values))

            line1.set_data(time_points, oy_values)
            line2.set_data(time_points, oz_values)

            ax1.set_xlim(0, len(oy_values))
            ax2.set_xlim(0, len(oz_values))

            plt.draw()
            plt.pause(0.5)
        except ValueError:
            print("跳过格式错误的行：", row)

# 关闭交互式模式，最终显示图像
plt.ioff()
plt.show()
