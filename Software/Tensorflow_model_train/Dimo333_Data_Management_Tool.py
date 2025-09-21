# """
# Filename: DIMO333_Data_Visualization_Annotation_Tool.py
# Developer: dimo333
# Created: 20250921
# Version: 3.0

# Description:
# IMU Data Visualization and Annotation Tool
# A tool for loading, visualizing, filtering, and annotating IMU sensor data 
# to confirm, delete, mark, and compare data samples, with support for saving the filtered dataset.

# Key Features:
# - Visualize accelerometer data with interactive plotting
# - Interactive data annotation (Confirm/Delete/Mark)
# - Save filtered datasets with preserved data integrity

# Copyright (c) dimo333
# """

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Button
import os
import sys
import io

# 设置标准输出使用UTF-8编码
if sys.stdout.encoding != 'UTF-8':
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

# 设置中文字体（仅用于图形界面）
plt.rcParams['font.sans-serif'] = ['SimHei']  # 使用黑体
plt.rcParams['axes.unicode_minus'] = False    # 解决负号显示问题

# 替换为您的文件路径
file_path = 'E:/Dtemp/Electromagic_Wand_ESP32-main/Electromagic_Wand_ESP32-main/Electromagic_Wand_Software/Basic/Tensorflow_model_train/show.csv'

# 读取数据时指定数据类型为浮点数，确保精度
data = np.loadtxt(file_path, delimiter=',', dtype=float)

# 打印数据的形状，查看加载后的数据维度
print("Data shape:", data.shape)

# 验证数据是 (50, 400)
assert data.shape[1] == 400, "Each row should have 400 data points (y0,z0,y1,z1...)"

# 提取 y 和 z 轴数据（交错列）
y_data = data[:, ::2]  # shape: (50, 200)
z_data = data[:, 1::2]  # shape: (50, 200)

# 构建统一横坐标（200个点代表2秒，采样率100Hz）
time = np.linspace(0, 2, 200)

# 创建图形和子图
fig = plt.figure(figsize=(15, 8))
ax = plt.subplot2grid((1, 10), (0, 0), colspan=7)  # 左侧图表区域
ax_marked = plt.subplot2grid((1, 10), (0, 7), colspan=3)  # 右侧标记区域

plt.subplots_adjust(bottom=0.25)  # 为按钮留出更多空间

# 初始化变量
current_index = 0
confirmed_indices = []  # 存储已确认的数据索引
deleted_indices = []    # 存储已删除的数据索引
marked_indices = []     # 存储已标记的数据索引
compare_mode = False    # 对比模式标志

# 创建按钮 - 第一行
ax_confirm = plt.axes([0.1, 0.05, 0.12, 0.075])
ax_delete = plt.axes([0.25, 0.05, 0.12, 0.075])
ax_next = plt.axes([0.4, 0.05, 0.12, 0.075])
ax_mark = plt.axes([0.55, 0.05, 0.12, 0.075])
ax_save = plt.axes([0.7, 0.05, 0.12, 0.075])

# 创建按钮 - 第二行（新增对比按钮）
ax_compare = plt.axes([0.1, 0.15, 0.12, 0.075])

btn_confirm = Button(ax_confirm, '确认Confirm')
btn_delete = Button(ax_delete, '删除Delete')
btn_next = Button(ax_next, '下一组Next')
btn_mark = Button(ax_mark, '标记Mark')
btn_save = Button(ax_save, '保存Save')
btn_compare = Button(ax_compare, '对比Compare')  # 新增对比按钮

# 保存所有按钮引用（除了对比按钮）
all_buttons = [btn_confirm, btn_delete, btn_next, btn_mark, btn_save]

# 绘制函数
def plot_data():
    ax.clear()
    
    # 绘制已确认的数据（使用更淡的颜色）
    for idx in confirmed_indices:
        ax.plot(time, y_data[idx], color='lightblue', alpha=0.3, linewidth=1.5)  # 淡蓝色
        ax.plot(time, z_data[idx], color='lightcoral', alpha=0.3, linewidth=1.5)  # 淡红色
    
    # 如果不是对比模式，绘制当前数据（黄色和绿色，保持鲜艳以便区分）
    if not compare_mode and current_index not in deleted_indices:
        ax.plot(time, y_data[current_index], color='yellow', label='Y-axis (Unconfirmed)', linewidth=2)
        ax.plot(time, z_data[current_index], color='green', label='Z-axis (Unconfirmed)', linewidth=2)
    
    # 添加图例
    ax.legend(loc='upper right')
    
    # 标题和标签
    mode_text = " (对比模式)" if compare_mode else ""
    ax.set_title(f'dimo333_数据可视化工具 - 当前: {current_index+1}/{len(y_data)} (确认: {len(confirmed_indices)}, 删除: {len(deleted_indices)}){mode_text}')
    ax.set_xlabel('Time (seconds)')
    ax.set_ylabel('Acceleration')
    ax.grid(True)
    
    # 更新标记区域
    update_marked_list()
    
    plt.draw()

# 更新标记列表
def update_marked_list():
    ax_marked.clear()
    ax_marked.set_title('被标记的数据')
    ax_marked.set_xlim(0, 1)
    ax_marked.set_ylim(0, 1)
    ax_marked.axis('off')  # 隐藏坐标轴
    
    if marked_indices:
        # 显示已标记的索引
        for i, idx in enumerate(marked_indices):
            ax_marked.text(0.1, 0.9 - i*0.05, f'Index: {idx}', fontsize=12)
    else:
        ax_marked.text(0.3, 0.5, '没有被标记的数据', fontsize=14, color='gray')

# 切换按钮可见性
def toggle_button_visibility():
    for button in all_buttons:
        if compare_mode:
            # 对比模式下隐藏按钮
            button.ax.set_visible(False)
        else:
            # 非对比模式下显示按钮
            button.ax.set_visible(True)
    plt.draw()

# 对比模式切换函数
def toggle_compare(event):
    global compare_mode
    compare_mode = not compare_mode  # 切换对比模式
    status = "开启" if compare_mode else "关闭"
    print(f"对比模式{status}")
    
    # 切换按钮可见性
    toggle_button_visibility()
    
    plot_data()

# 按钮回调函数
def confirm(event):
    if compare_mode:
        return  # 对比模式下忽略其他按钮点击
    global confirmed_indices
    if current_index not in confirmed_indices and current_index not in deleted_indices:
        confirmed_indices.append(current_index)
        print(f"Confirmed data index: {current_index}")
        plot_data()

def delete(event):
    if compare_mode:
        return  # 对比模式下忽略其他按钮点击
    global deleted_indices
    if current_index not in deleted_indices:
        deleted_indices.append(current_index)
        print(f"Deleted data index!!!!!!!!!!!: {current_index}")
        # 如果当前数据已确认，也需要从确认列表中移除
        if current_index in confirmed_indices:
            confirmed_indices.remove(current_index)
        # 如果当前数据已标记，也需要从标记列表中移除
        if current_index in marked_indices:
            marked_indices.remove(current_index)
        plot_data()

def next_data(event):
    if compare_mode:
        return  # 对比模式下忽略其他按钮点击
    global current_index
    # 查找下一个未删除的数据
    while True:
        current_index = (current_index + 1) % len(y_data)
        if current_index not in deleted_indices:
            break
        # 如果所有数据都已删除，退出循环
        if len(deleted_indices) >= len(y_data):
            print("All data have been deleted!")
            break
    
    plot_data()

def mark_data(event):
    if compare_mode:
        return  # 对比模式下忽略其他按钮点击
    global marked_indices
    if current_index not in marked_indices and current_index not in deleted_indices:
        marked_indices.append(current_index)
        print(f"Marked data index------------------------> {current_index}<------------------------")
        update_marked_list()
        plt.draw()

def save_changes(event):
    if compare_mode:
        return  # 对比模式下忽略其他按钮点击
    global data, y_data, z_data, current_index, confirmed_indices, marked_indices, deleted_indices
    
    # 创建保留的数据索引
    kept_indices = [i for i in range(len(data)) if i not in deleted_indices]
    
    # 提取保留的数据
    filtered_data = data[kept_indices, :]
    
    # 保存筛选后的数据到原文件，使用更通用的格式
    # 使用 '%g' 格式，它会自动选择最合适的表示方式
    np.savetxt(file_path, filtered_data, delimiter=',', fmt='%g')
    
    # 重新读取数据以更新全局变量
    data = np.loadtxt(file_path, delimiter=',', dtype=float)
    y_data = data[:, ::2]
    z_data = data[:, 1::2]
    
    # 重置索引
    current_index = 0
    confirmed_indices = []
    marked_indices = []
    deleted_indices = []
    print(f"Changes saved! Original data: {len(data)} rows, After filtering: {len(filtered_data)} rows")
    
    # 重新绘制
    plot_data()

# 连接按钮和回调函数
btn_confirm.on_clicked(confirm)
btn_delete.on_clicked(delete)
btn_next.on_clicked(next_data)
btn_mark.on_clicked(mark_data)
btn_save.on_clicked(save_changes)
btn_compare.on_clicked(toggle_compare)  # 连接对比按钮

# 初始绘制
plot_data()

# 显示图形
plt.show()