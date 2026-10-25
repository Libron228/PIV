#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Wire.h>

// ===============================
// НАСТРОЙКИ WI-FI
// ===============================
const char* AP_SSID = "R2D2-Control";
const char* AP_PASSWORD = "password123";

WebServer server(80);
bool isDancing = false;

// ===============================
// HTML-СТРАНИЦА
// ===============================
const char MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>R2-D2 Control</title>
<style>
  body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: #1a1a2e;
    color: white;
    text-align: center;
    margin: 0;
    padding: 30px 15px;
  }
  h1 { margin-bottom: 40px; color: #0f3460; text-shadow: 1px 1px 2px #e94560; font-size: 2.5em;}
  .btn {
    display: block;
    width: 80%;
    max-width: 300px;
    margin: 20px auto;
    padding: 20px;
    font-size: 24px;
    font-weight: bold;
    color: white;
    border: none;
    border-radius: 12px;
    cursor: pointer;
    box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    transition: transform 0.1s;
  }
  .btn:active { transform: scale(0.95); }
  .btn-start { background: linear-gradient(135deg, #00b09b, #96c93d); }
  .btn-stop { background: linear-gradient(135deg, #e52d27, #b31217); }
</style>
</head>
<body>
  <h1>R2-D2</h1>
  <button class="btn btn-start" onclick="sendCommand('/dance/on')">ТАНЦЕВАТЬ</button>
  <button class="btn btn-stop" onclick="sendCommand('/dance/off')">СТОП</button>

  <script>
    function sendCommand(url) {
      fetch(url).catch(err => console.error(err));
    }
  </script>
</body>
</html>
)rawliteral";

// ===============================
// ПИНЫ И НАСТРОЙКИ РОБОТА
// ===============================
const int IN1 = 16;
const int IN2 = 17;
const int IN3 = 18;
const int IN4 = 19;

const int PIN_SERVO_HEAD  = 25;
const int PIN_SERVO_LEG_L = 26;
const int PIN_SERVO_LEG_R = 27;

// Оставляем только рабочий канал
const int PIN_LED = 23; 
const int PIN_BUZZER = 4;

const int MPU_ADDR = 0x68;
const float IMPACT_THRESHOLD = 26000.0;

Servo servoHead;
Servo servoLegL;
Servo servoLegR;

// Переменные для хранения текущего угла
int currentHeadAngle = 0;
int currentTiltAngle = 90;

// Предварительное объявление функций
void moveRobot(int speedLeft, int speedRight);
void setLEDBrightness(int brightness);
void setRandomBrightness();
bool checkCollision();
void playEmotionShock();
bool waitAndCheck(unsigned long ms);
void danceRoutine();
void resetPosition();

void r2Chirp(int times);
void r2Whistle();
void r2SadTone();

bool smoothRotateHead(int targetAngle, int speedDelay = 15);
void instantRotateHead(int angle);
void setBodyTilt(int angle);

// ===============================
// ОБРАБОТЧИКИ ВЕБ-СЕРВЕРА
// ===============================
void handleRoot() {
  server.send(200, "text/html", MAIN_PAGE);
}

void handleDanceOn() {
  isDancing = true;
  server.send(200, "text/plain", "OK");
}

void handleDanceOff() {
  isDancing = false;
  resetPosition(); 
  server.send(200, "text/plain", "OK");
}

// ===============================
// SETUP
// ===============================
void setup() {
  Serial.begin(115200);

  // Настройка пинов
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Сервоприводы
  servoHead.attach(PIN_SERVO_HEAD, 500, 2400);
  servoLegL.attach(PIN_SERVO_LEG_L, 500, 2400);
  servoLegR.attach(PIN_SERVO_LEG_R, 500, 2400);

  // I2C для акселерометра
  Wire.begin(21, 22);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00);
  Wire.endTransmission();

  // Исходная позиция
  resetPosition();

  // Запуск Wi-Fi
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  server.on("/", handleRoot);
  server.on("/dance/on", handleDanceOn);
  server.on("/dance/off", handleDanceOff);
  server.begin();
  
  delay(1000);
}

// ===============================
// LOOP
// ===============================
void loop() {
  server.handleClient(); 

  if (isDancing) {
    danceRoutine(); 
  } else {
    if (checkCollision()) {
      playEmotionShock();
    }
  }
}

// ===============================
// УМНАЯ ЗАДЕРЖКА 
// ===============================
bool waitAndCheck(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient(); 

    if (!isDancing) return true; 
    
    if (checkCollision()) {
      playEmotionShock(); 
      return true;        
    }
    delay(10);
  }
  return false;
}

// ================= СЦЕНАРИЙ: БОЛЬШОЙ ТАНЕЦ =================
void danceRoutine() {
  if (!isDancing) return;

  // --- ЧАСТЬ 1: Приветствие и осмотр ---
  r2Whistle();
  setRandomBrightness();
  
  if (smoothRotateHead(-45, 15)) return;
  r2Chirp(2);
  if (smoothRotateHead(45, 15)) return;
  r2Chirp(2);
  if (smoothRotateHead(0, 15)) return;

  // --- ЧАСТЬ 2: Движение с резкими наклонами ---
  for (int i = 0; i < 2; i++) {
    setRandomBrightness();
    setBodyTilt(70); // Резкий наклон вперед
    moveRobot(200, 200);
    r2Whistle();
    if (waitAndCheck(800)) return;
    moveRobot(0, 0);

    setRandomBrightness();
    setBodyTilt(110); // Резкий наклон назад
    moveRobot(-200, -200);
    r2Chirp(4);
    if (waitAndCheck(800)) return;
    moveRobot(0, 0);
  }
  
  setBodyTilt(90);

  // --- ЧАСТЬ 3: Дискотека на месте ---
  for (int i = 0; i < 4; i++) {
    setRandomBrightness();
    
    moveRobot((i%2 == 0) ? 255 : -255, (i%2 == 0) ? -255 : 255);
    
    if (smoothRotateHead((i%2 == 0) ? 50 : -50, 10)) return;
    r2Chirp(3);
    
    moveRobot(0, 0);
    if (waitAndCheck(300)) return;
  }

  // --- ЧАСТЬ 4: Завершение круга ---
  setLEDBrightness(255); // Яркое свечение
  if (smoothRotateHead(0, 15)) return;
  setBodyTilt(90);
  r2Whistle();
  
  if (waitAndCheck(2000)) return; 
}

// ================= РЕАКЦИЯ НА СТОЛКНОВЕНИЕ (ГРУСТЬ) =================
void playEmotionShock() {
  moveRobot(0, 0); 
  
  // 1. Испуг (резкие максимальные вспышки)
  setLEDBrightness(255); 
  instantRotateHead(45);
  tone(PIN_BUZZER, 2500, 200); delay(200);
  
  setLEDBrightness(0); // Погасить
  instantRotateHead(-45);
  tone(PIN_BUZZER, 2000, 200); delay(200);
  
  setLEDBrightness(255); 
  noTone(PIN_BUZZER);
  instantRotateHead(0);

  // 2. Уходим в грусть
  setLEDBrightness(30);        // Тусклое свечение (грусть)
  setBodyTilt(60);             // Резкий наклон корпуса
  r2SadTone();                 // Грустный звук

  // 3. Грустим 5 секунд
  unsigned long sadnessStart = millis();
  while(millis() - sadnessStart < 5000) {
     server.handleClient();
     if (!isDancing) return; 
     delay(10);
  }

  // 4. Оживаем
  if (isDancing) {
    setBodyTilt(90); 
    setLEDBrightness(200); // Возвращаем яркий свет
    r2Whistle();
  }
}

// ================= ЗВУКИ R2-D2 =================

void r2Chirp(int times) {
  for (int i = 0; i < times; i++) {
    setRandomBrightness(); // Мерцание при каждом "чирике"
    int freq = random(1500, 3500);
    tone(PIN_BUZZER, freq, 80);
    if (waitAndCheck(100)) { noTone(PIN_BUZZER); return; }
    noTone(PIN_BUZZER);
  }
}

void r2Whistle() {
  for (int i = 1000; i < 2500; i += 100) {
    setRandomBrightness(); // Мерцание во время свиста
    tone(PIN_BUZZER, i, 30);
    if (waitAndCheck(20)) { noTone(PIN_BUZZER); return; }
  }
  noTone(PIN_BUZZER);
}

void r2SadTone() {
  for(int freq = 1800; freq > 400; freq -= 40) {
    tone(PIN_BUZZER, freq, 60);
    delay(50); 
  }
  noTone(PIN_BUZZER);
}

// ================= ДВИЖЕНИЯ СЕРВО =================

bool smoothRotateHead(int targetAngle, int speedDelay) {
  targetAngle = constrain(targetAngle, -60, 60);
  while (currentHeadAngle != targetAngle) {
    if (currentHeadAngle < targetAngle) currentHeadAngle++;
    else currentHeadAngle--;
    
    int servoAngle = map(currentHeadAngle, -60, 60, 30, 150);
    servoHead.write(servoAngle);
    
    if (waitAndCheck(speedDelay)) return true;
  }
  return false;
}

void instantRotateHead(int angle) {
  currentHeadAngle = constrain(angle, -60, 60);
  servoHead.write(map(currentHeadAngle, -60, 60, 30, 150));
}

// Быстрый наклон корпуса
void setBodyTilt(int angle) {
  currentTiltAngle = constrain(angle, 45, 135);
  servoLegL.write(currentTiltAngle);
  servoLegR.write(180 - currentTiltAngle);
}

// ================= БАЗОВЫЕ ФУНКЦИИ =================

void resetPosition() {
  moveRobot(0, 0);
  instantRotateHead(0);
  setBodyTilt(90);
  setLEDBrightness(0); // Выключить ленту
  noTone(PIN_BUZZER);
}

// Управление яркостью по ШИМ (0-255)
void setLEDBrightness(int brightness) {
  brightness = constrain(brightness, 0, 255);
  analogWrite(PIN_LED, brightness);
}

// Случайная яркость для эффекта живого мерцания
void setRandomBrightness() {
  // Выдаём значение от 50 до 255, чтобы он не угасал полностью
  int brightness = random(50, 256); 
  setLEDBrightness(brightness);
}

void moveRobot(int speedLeft, int speedRight) {
  if (speedLeft > 0) {
    analogWrite(IN1, speedLeft); analogWrite(IN2, 0);
  } else if (speedLeft < 0) {
    analogWrite(IN1, 0); analogWrite(IN2, abs(speedLeft));
  } else {
    analogWrite(IN1, 0); analogWrite(IN2, 0);
  }

  if (speedRight > 0) {
    analogWrite(IN3, speedRight); analogWrite(IN4, 0);
  } else if (speedRight < 0) {
    analogWrite(IN3, 0); analogWrite(IN4, abs(speedRight));
  } else {
    analogWrite(IN3, 0); analogWrite(IN4, 0);
  }
}

bool checkCollision() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  
  Wire.requestFrom(MPU_ADDR, 6, true);
  if (Wire.available() >= 6) {
    int16_t rawX = (Wire.read() << 8) | Wire.read();
    int16_t rawY = (Wire.read() << 8) | Wire.read();
    int16_t rawZ = (Wire.read() << 8) | Wire.read();

    float totalAccel = sqrt((float)rawX * rawX + 
                            (float)rawY * rawY + 
                            (float)rawZ * rawZ);

    if (totalAccel > IMPACT_THRESHOLD) {
      return true;
    }
  }
  return false;
}