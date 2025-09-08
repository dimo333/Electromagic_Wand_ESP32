import numpy as np
import matplotlib.pyplot as plt

# 替换为你的文件路径
file_path = 'E:/Dtemp/Electromagic_Wand_ESP32-main/Electromagic_Wand_ESP32-main/Electromagic_Wand_Software/Basic/Tensorflow_model_train/show.csv'
data = np.loadtxt(file_path, delimiter=',')

# 打印数据的形状，查看加载后的数据维度
print("Data shape:", data.shape)

# 进一步检查每一行的数据量
print("First row of data:", data[0])

# 验证数据是 (50, 400)
assert data.shape[1] == 400, "每行应为400个数据(y0,z0,y1,z1...)"

# 提取 y 和 z 轴数据（交错列）
y_data = data[:, ::2]  # shape: (50, 200)
z_data = data[:, 1::2]  # shape: (50, 200)

# 构建统一横坐标（200个点代表2秒，采样率100Hz）
time = np.linspace(0, 2, 200)

# 开始绘图
plt.figure(figsize=(12, 6))

# 绘制每一组数据
for i in range(len(y_data)):
    # 使用 alpha 设置透明度避免重叠
    plt.plot(time, y_data[i], color='blue', alpha=0.2)  # Y轴蓝色
    plt.plot(time, z_data[i], color='red', alpha=0.2)   # Z轴红色

# 标题、坐标标签和网格
plt.title('50 Gesture Groups - Y and Z Axis', fontsize=16)
plt.xlabel('Time (seconds)', fontsize=14)
plt.ylabel('Acceleration', fontsize=14)
plt.grid(True)

# 调整布局以避免标签重叠
plt.tight_layout()

# 显示图形
plt.show()
