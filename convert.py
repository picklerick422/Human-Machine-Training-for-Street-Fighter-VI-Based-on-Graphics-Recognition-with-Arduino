#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# 修改后的按键到舵机编号映射
KEY_SERVO_MAP = {
    'A': 0, 'S': 1, 'D': 2, 'H': 3, 'J': 4, 
    'K': 5, 'W': 6, 'U': 7, 'I': 8, 'O': 9
}

def parse_action_line(line):
    """解析一行搓招序列，返回舵机命令列表"""
    line = line.strip()
    if not line:
        return []
    
    # 处理长按标记 ~
    processed_line = line.replace('~', ' ~ ')
    segments = processed_line.split()
    
    commands = []
    
    for segment in segments:
        if segment == '~':
            commands.append("(0,0,1)")
        else:
            servo_ids = [str(KEY_SERVO_MAP[k]) for k in segment if k in KEY_SERVO_MAP]
            if servo_ids:
                press_cmd = "".join([f"({sid},2,5)" for sid in servo_ids])
                commands.append(press_cmd)
    
    # 归位命令
    all_keys = [c for c in line if c in KEY_SERVO_MAP]
    if all_keys:
        servo_ids = [str(KEY_SERVO_MAP[k]) for k in set(all_keys)]
        release_cmd = "".join([f"({sid},-2,5)" for sid in servo_ids])
        commands.append(release_cmd)
    
    return commands

def main():
    try:
        with open("action.txt", "r", encoding="utf-8") as f_in:
            with open("servo_commands.txt", "w", encoding="utf-8") as f_out:
                for line_num, line in enumerate(f_in, 1):
                    line = line.strip()
                    if not line:
                        continue
                    
                    commands = parse_action_line(line)
                    if commands:
                        f_out.write("\n".join(commands) + "\n\n")
                    
                    print(f"第{line_num}行 '{line}' -> 生成 {len(commands)} 条命令")
        
        print("\n转换完成！已保存到 servo_commands.txt")
        
    except FileNotFoundError:
        print("错误：找不到 action.txt 文件")
    except Exception as e:
        print(f"发生错误: {e}")

if __name__ == "__main__":
    main()