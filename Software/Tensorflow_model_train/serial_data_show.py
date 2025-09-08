import serial
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# 配置串口参数
port = 'COM38'  # ///////////////////////////////////////////////////////////////替换为你的ESP32连接的串口号
baud_rate = 115200  # ///////////////////////////////////////////////////////////替换为你的ESP32的波特率

data_x = 'data_x.csv'  # ///////////////////////////////////////////////////////////替换为你要存放动作数据的文件
data_y = 'data_y.csv'  # ///////////////////////////////////////////////////////////替换为你要存放数据对应动作分类的文件
label = [6]  # ////////////////////////////////////////////////////////////////////方框里改成你要录入的动作的序号，比如我现在要录入第一个动作，那就写0，类推。

num = 0

# 配置数据可视化
plt.figure(figsize=(12, 6))
time = np.linspace(0, 2, 200)  # 时间轴，200个点表示2秒，采样率100Hz

try:
    # 打开串口
    ser = serial.Serial(port, baud_rate)
    print(f"串口 {port} 打开成功，波特率 {baud_rate}")

    # 打开或创建CSV文件
    with open(data_x, mode='a', newline='') as file1, open(data_y, mode='a', newline='') as file2:
        while True:
            # 读取串口数据
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                print(f"读取到数据: {line}")  # 打印读取到的数据

                try:
                    # 将数据按逗号分割并转换为float32
                    data = [float(x) for x in line.split(',')]

                    # 将数据转换为DataFrame
                    df_x = pd.DataFrame([data])
                    df_y = pd.DataFrame([label])

                    # 追加数据到CSV文件
                    df_x.to_csv(file1, header=False, index=False)
                    df_y.to_csv(file2, header=False, index=False)

                    # 刷新文件缓冲区
                    file1.flush()
                    file2.flush()

                    # 显示追加数据成功
                    print(f"数据追加到CSV文件成功: {num}")
                    num += 1

                    # 从CSV文件中加载所有数据
                    data_loaded = pd.read_csv(data_x, header=None).values

                    # 提取 Y 和 Z 轴数据（交错列）
                    y_data = data_loaded[:, ::2]  # Y轴数据：每隔一个取一个，即偶数列
                    z_data = data_loaded[:, 1::2]  # Z轴数据：每隔一个取一个，即奇数列

                    # 绘制 Y 和 Z 轴数据
                    plt.clf()  # 清空当前图形
                    for i in range(len(y_data)):
                        plt.plot(time, y_data[i], color='blue', alpha=0.2)  # Y轴蓝色
                        plt.plot(time, z_data[i], color='red', alpha=0.2)   # Z轴红色

                    # 更新图表
                    plt.title('50 Gesture Groups - Y and Z Axis', fontsize=16)
                    plt.xlabel('Time (seconds)', fontsize=14)
                    plt.ylabel('Acceleration', fontsize=14)
                    plt.grid(True)
                    plt.tight_layout()
                    plt.pause(0.1)  # 暂停以刷新图像

                except ValueError as e:
                    print(f"数据转换错误: {e}")

except serial.SerialException as e:
    print(f"串口打开失败: {e}")
except Exception as e:
    print(f"发生错误: {e}")
