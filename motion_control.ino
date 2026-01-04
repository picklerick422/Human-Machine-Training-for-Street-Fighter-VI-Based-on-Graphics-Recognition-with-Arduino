/***********************************************************************
 * Arduino UNO R4 Minima - 360度舵机控制（最小修改版）
 * 输入格式：(舵机ID,旋转角度,转速等级)(舵机ID,旋转角度,转速等级)...
 */

// 仅修改引脚定义（硬件PWM引脚，信号更稳定）
const int SERVO_PINS[] = {2, 3, 4, 5, 6, 7, 8, 9, 10};
const int SERVO_COUNT = sizeof(SERVO_PINS) / sizeof(SERVO_PINS[0]);

struct ServoState {
  int targetSteps;
  int currentSteps;
  int speedLevel;
  int direction;
  bool isMoving;
  unsigned long lastPulseTime;  // 新增：每个舵机独立计时
};

ServoState servoStates[SERVO_COUNT];
String inputString = "";
bool newCommandReceived = false;

void setup()
{
  for (int i = 0; i < SERVO_COUNT; i++) {
    pinMode(SERVO_PINS[i], OUTPUT);
    servoStates[i].isMoving = false;
    servoStates[i].lastPulseTime = 0;  // 初始化时间
  }
  
  Serial.begin(9600);
  delay(1000);
  Serial.println("=== 多舵机控制器 ===");
}

void loop()
{
  checkSerialInput();
  
  if (newCommandReceived) {
    processSerialData();
    inputString = "";
    newCommandReceived = false;
  }
  
  updateServos();  // 核心：独立调度每个舵机
}

// 新增：解析多命令 (0,10,5)(1,-10,5)
void processSerialData() {
  inputString.trim();
  if (!inputString.startsWith("(") || !inputString.endsWith(")")) {
    Serial.println("错误：格式错误");
    return;
  }

  int start = 0;
  while (start < inputString.length()) {
    int openParen = inputString.indexOf('(', start);
    int closeParen = inputString.indexOf(')', start);
    if (openParen == -1 || closeParen == -1) break;
    
    String cmd = inputString.substring(openParen + 1, closeParen);
    parseCommand(cmd);
    start = closeParen + 1;
  }
}

// 解析单个命令
void parseCommand(String cmd) {
  int c1 = cmd.indexOf(',');
  int c2 = cmd.indexOf(',', c1 + 1);
  if (c1 == -1 || c2 == -1) return;

  int id = cmd.substring(0, c1).toInt();
  int angle = cmd.substring(c1 + 1, c2).toInt();
  int speed = cmd.substring(c2 + 1).toInt();
  
  if (id >= 0 && id < SERVO_COUNT && speed >= 1 && speed <= 10) {
    // 设置舵机状态（所有舵机同时开始）
    servoStates[id].targetSteps = abs(angle) * 2;  // 每步0.5度
    servoStates[id].currentSteps = 0;
    servoStates[id].speedLevel = speed;
    servoStates[id].direction = (angle > 0) ? 1 : -1;
    servoStates[id].isMoving = true;
    servoStates[id].lastPulseTime = millis();  // 记录开始时间
  }
}

// 核心修改：每个舵机独立判断周期
void updateServos() {
  unsigned long now = millis();
  
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (!servoStates[i].isMoving) continue;

    if (servoStates[i].currentSteps >= servoStates[i].targetSteps) {
      servopulse(SERVO_PINS[i], 1500);  // 停止
      servoStates[i].isMoving = false;
      Serial.println("舵机" + String(i) + "完成");
      continue;
    }

    // 每个舵机独立判断20ms周期
    if (now - servoStates[i].lastPulseTime >= 5) {
      int pulse = (servoStates[i].direction == 1) 
        ? map(servoStates[i].speedLevel, 1, 10, 2000, 2500)
        : map(servoStates[i].speedLevel, 1, 10, 900, 400);
      
      // 每次发8个脉冲（确保平滑）
      for (int p = 0; p < 4; p++) {
        servopulse(SERVO_PINS[i], pulse);
      }
      
      servoStates[i].currentSteps += 4;
      servoStates[i].lastPulseTime = now;
    }
  }
}

void servopulse(int pin, int pulsewidth) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(pulsewidth);
  digitalWrite(pin, LOW);
  delayMicroseconds(20000 - pulsewidth);
}

void checkSerialInput() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      newCommandReceived = true;
      break;
    }
    inputString += c;
  }
}