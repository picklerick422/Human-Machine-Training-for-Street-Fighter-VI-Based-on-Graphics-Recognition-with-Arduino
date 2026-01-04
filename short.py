#/***********************************************************************
# * Python 舵机指令发送器
# * 功能：读取本地 servo_commands.txt 文件，逐行发送给 Arduino
# * 安装依赖：pip install pyserial
# ***********************************************************************/

import serial
import time
import re

# ====== 配置 ======
SERIAL_PORT = "COM9"  # Windows 串口名，Linux/Mac 可能是 "/dev/ttyUSB0"
BAUD_RATE = 9600
COMMAND_FILE = "short.txt"  # 本地指令文件路径

# ====== 初始化串口 ======
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    #time.sleep(2)  # 等待 Arduino 初始化
    print(f"已连接到 {SERIAL_PORT}")
except Exception as e:
    print(f"串口连接失败: {e}")
    exit()

# ====== 读取文件并发送指令 ======
def send_commands_from_file(file_path):
    try:
        with open(file_path, "r", encoding="utf-8") as f:
            lines = f.readlines()
        
        for line_num, line in enumerate(lines, start=1):
            line = line.strip()
            if not line:
                continue  # 跳过空行
            
            # 用正则匹配指令 (id,angle,speed)
            pattern = r"\((\d+),(-?\d+),(\d+)\)"
            commands = re.findall(pattern, line)
            
            if not commands:
                print(f"第 {line_num} 行格式错误，跳过: {line}")
                continue
            
            # 发送整行指令给 Arduino
            ser.write((line + "\n").encode("utf-8"))
            print(f"发送指令行 {line_num}: {line}")
            
            # 等待 Arduino 响应或执行完成
            #time.sleep(0.5)  # 可根据舵机动作时间调整
            
            # 可选：读取 Arduino 返回信息
            response = ser.readline().decode("utf-8").strip()
            if response:
                print(f"Arduino 响应: {response}")
    
    except FileNotFoundError:
        print(f"文件 {file_path} 不存在")
    except Exception as e:
        print(f"读取文件时发生错误: {e}")

# ====== 主程序 ======
if __name__ == "__main__":
    print("开始发送指令...")
    send_commands_from_file(COMMAND_FILE)
    print("所有指令发送完毕")
    ser.close()
