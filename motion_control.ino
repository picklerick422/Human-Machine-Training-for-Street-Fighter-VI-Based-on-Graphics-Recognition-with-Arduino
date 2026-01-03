/***********************************************************************
 * Arduino UNO R4 Minima - 360度舵机控制（带旋转角度和固定转速）
 * 数据格式：舵机编号,旋转角度,转速等级 （如 "1,720,5" 或 "1,-360,10"）
 * 旋转角度：任意整数（正=正转，负=反转，如720=转2圈）
 * 转速等级：1-10（1最慢，10最快）
 */

// 定义舵机引脚
const int SERVO_PINS[] = {7, 8, 9, 10, 11};
const int SERVO_COUNT = sizeof(SERVO_PINS) / sizeof(SERVO_PINS[0]);

// 舵机状态结构体
struct ServoState {
  int targetSteps;      // 总旋转步数（由角度转换）
  int currentSteps;     // 当前已执行步数
  int speedLevel;       // 转速等级1-10
  int direction;        // 方向：1=正转, -1=反转, 0=停止
  bool isMoving;        // 是否正在旋转
};

ServoState servoStates[SERVO_COUNT];
String inputString = "";
unsigned long lastReceiveTime = 0;
bool newCommandReceived = false;

void setup()
{
  // 初始化舵机引脚和状态
  for (int i = 0; i < SERVO_COUNT; i++)
  {
    pinMode(SERVO_PINS[i], OUTPUT);
    servoStates[i].isMoving = false;
    servoStates[i].direction = 0;
  }
  
  Serial.begin(9600);
  delay(1000);
  Serial.println("=== 360度舵机控制器已启动 ===");
  Serial.println("数据格式：舵机编号,旋转角度,转速等级");
  Serial.println("示例：1,720,5  （舵机1正转2圈，中速）");
  Serial.println("示例：1,-360,10 （舵机1反转1圈，高速）");
  Serial.print("支持的电机编号：0-");
  Serial.println(SERVO_COUNT - 1);
  Serial.println("等待命令...");
}

void loop()
{
  // 1. 检查串口输入
  checkSerialInput();
  
  // 2. 处理新命令
  if (newCommandReceived)
  {
    processSerialData();
    inputString = "";
    newCommandReceived = false;
  }
  
  // 3. 持续更新所有舵机（非阻塞式）
  updateServos();
  
  delay(20);  // 保持50Hz刷新率
}

void checkSerialInput()
{
  while (Serial.available())
  {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r')
    {
      lastReceiveTime = millis();
      newCommandReceived = true;
      break;
    }
    else
    {
      inputString += inChar;
      lastReceiveTime = millis();
    }
  }
}

void processSerialData()
{
  inputString.trim();
  if (inputString.length() == 0) return;

  // 解析三个参数：舵机编号,旋转角度,转速等级
  int firstComma = inputString.indexOf(',');
  if (firstComma == -1)
  {
    Serial.println("错误：格式不正确！需要用逗号分隔");
    return;
  }

  int secondComma = inputString.indexOf(',', firstComma + 1);
  if (secondComma == -1)
  {
    Serial.println("错误：缺少转速等级！格式：舵机编号,旋转角度,转速等级");
    return;
  }

  int servoId = inputString.substring(0, firstComma).toInt();
  int rotationAngle = inputString.substring(firstComma + 1, secondComma).toInt();
  int speedLevel = inputString.substring(secondComma + 1).toInt();

  // 验证范围
  if (servoId < 0 || servoId >= SERVO_COUNT)
  {
    Serial.print("错误：电机编号超出范围！有效：0-");
    Serial.println(SERVO_COUNT - 1);
    return;
  }
  
  if (speedLevel < 1 || speedLevel > 10)
  {
    Serial.println("错误：转速等级超出范围！有效：1-10");
    return;
  }

  // 停止当前动作，设置新参数
  servoStates[servoId].isMoving = false;
  delay(50);  // 短暂停止确保切换

  // 计算参数
  // 每步=2度，可根据实际测试调整这个值
  servoStates[servoId].targetSteps = abs(rotationAngle) / 2;
  servoStates[servoId].currentSteps = 0;
  servoStates[servoId].speedLevel = speedLevel;
  servoStates[servoId].direction = (rotationAngle > 0) ? 1 : -1;
  servoStates[servoId].isMoving = true;

  Serial.print("→ 舵机 #");
  Serial.print(servoId);
  Serial.print(" 旋转角度：");
  Serial.print(rotationAngle);
  Serial.print("° 转速等级：");
  Serial.println(speedLevel);
}

// 核心函数：更新所有舵机状态
void updateServos()
{
  for (int i = 0; i < SERVO_COUNT; i++)
  {
    if (!servoStates[i].isMoving) continue;

    // 检查是否完成
    if (servoStates[i].currentSteps >= servoStates[i].targetSteps)
    {
      servopulse(SERVO_PINS[i], 1500);  // 发送停止信号
      servoStates[i].isMoving = false;
      servoStates[i].direction = 0;
      Serial.print("  舵机 #");
      Serial.print(i);
      Serial.println(" 旋转完成");
      continue;
    }

    // 计算脉冲宽度（基于转速等级和方向）
    int pulsewidth;
    if (servoStates[i].direction == 1)  // 正转
    {
      pulsewidth = map(servoStates[i].speedLevel, 1, 10, 1600, 2500);
    }
    else  // 反转
    {
      pulsewidth = map(servoStates[i].speedLevel, 1, 10, 1400, 500);
    }

    // 发送PWM信号并累加步数
    servopulse(SERVO_PINS[i], pulsewidth);
    servoStates[i].currentSteps++;
  }
}

// PWM脉冲产生函数
void servopulse(int pin, int pulsewidth)
{
  digitalWrite(pin, HIGH);
  delayMicroseconds(pulsewidth);
  digitalWrite(pin, LOW);
  delayMicroseconds(20000 - pulsewidth);
}