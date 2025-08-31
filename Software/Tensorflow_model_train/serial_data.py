import serial 
import pandas as pd

# 配置串口参数
port = 'COM14'  # ///////////////////////////////////////////////////////////////替换为你的ESP32连接的串口号
baud_rate = 115200  # ///////////////////////////////////////////////////////////替换为你的ESP32的波特率

data_x = 'data_x.csv'#///////////////////////////////////////////////////////////替换为你要存放动作数据的文件
data_y = 'data_y.csv'#///////////////////////////////////////////////////////////替换为你要存放数据对应动作分类的文件，比如你录入的是动作1，那么文档里就会存储序号0，类推。
label = [5] #////////////////////////////////////////////////////////////////////方框里改成你要录入的动作的序号，比如我现在要录入第一个动作，那就写0，类推。

num = 0

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
                    print(f"数据追加到CSV文件成功:{num}")
                    num += 1

                except ValueError as e:
                    print(f"数据转换错误: {e}")

except serial.SerialException as e:
    print(f"串口打开失败: {e}")
except Exception as e:
    print(f"发生错误: {e}")
