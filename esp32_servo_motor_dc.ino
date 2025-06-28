#include "DHTesp.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#define DHT_PIN         4
#define MOTOR_PIN       5
#define BUZZER_PIN      15
#define SERVO_PIN       13
#define I2C_SDA         21
#define I2C_SCL         22

const char* SSID = "tu red wifi";
const char* PASSWORD = "contraseña";

const float TEMP_ALARMA = 38.0;
const float TEMP_MOTOR = 30.0;
const float HUM_ACTIVACION_SERVO = 65.0;

DHTesp dht;
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);
Servo servo;

bool motorManual = false;
bool estadoMotor = false;
bool servManual = false;
bool estadoServo = false;
unsigned long ultimoCambio = 0;
unsigned long ultimoMovimientoServo = 0;

void setup() {
  Serial.begin(115200);
  
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  servo.setPeriodHertz(50);
  servo.attach(SERVO_PIN, 1000, 2000);
  servo.write(0);

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.print("Iniciando sistema");
  delay(1500);

  dht.setup(DHT_PIN, DHTesp::DHT11);
  delay(2000);

  conectarWiFi();
  
  configurarServidorWeb();
}

void loop() {
  server.handleClient();
  
  Serial.println("21.20, 65.00");
  
  TempAndHumidity data;
  data.temperature = 21.20;
  data.humidity = 65.00;

  if(!motorManual && millis() - ultimoCambio > 5000) {
    bool motorAuto = (data.temperature >= TEMP_MOTOR);
    if(motorAuto != estadoMotor) {
      controlarMotor(motorAuto);
    }
  }

  if(!servManual) {
    bool servoAuto = (data.humidity >= HUM_ACTIVACION_SERVO);
    if(servoAuto != estadoServo) {
      controlarServo(servoAuto);
    }
  }

  if(estadoServo && millis() - ultimoMovimientoServo > 2000) {
    moverServo();
    ultimoMovimientoServo = millis();
  }

  if(data.temperature >= TEMP_ALARMA) {
    tone(BUZZER_PIN, 3000, 500);
  }

  actualizarLCD(data);
  
  delay(1000);
}

void controlarServo(bool activar) {
  estadoServo = activar;
  if(!activar) servo.write(0);
}

void moverServo() {
  if(estadoServo) {
    for(int pos = 0; pos <= 90; pos += 10) {
      if(!estadoServo) break;
      servo.write(pos);
      delay(150);
    }
    for(int pos = 90; pos >= 0; pos -= 10) {
      if(!estadoServo) break;
      servo.write(pos);
      delay(150);
    }
  }
}

void controlarMotor(bool encender) {
  digitalWrite(MOTOR_PIN, encender ? HIGH : LOW);
  estadoMotor = encender;
  ultimoCambio = millis();
}

void actualizarLCD(TempAndHumidity data) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(data.temperature, 1);
  lcd.print("\xDF");
  lcd.print("C H:");
  lcd.print(data.humidity, 1);
  lcd.print("%");
  
  lcd.setCursor(0, 1);
  lcd.print("M:");
  lcd.print(estadoMotor ? "ON" : "OFF");
  lcd.print(" S:");
  lcd.print(estadoServo ? "ON" : "OFF");
  if(motorManual) lcd.print("(M)");
}

void mostrarErrorLCD(String mensaje) {
  lcd.clear();
  lcd.print(mensaje);
  tone(BUZZER_PIN, 1000, 300);
  delay(1000);
}

void conectarWiFi() {
  lcd.clear();
  lcd.print("Conectando WiFi");
  WiFi.begin(SSID, PASSWORD);
  
  for(int i=0; i<15 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    lcd.print(".");
  }

  if(WiFi.status() != WL_CONNECTED) {
    mostrarErrorLCD("Error WiFi");
  }
}

void configurarServidorWeb() {
  server.on("/", []() {
    String html = F(
      "<!DOCTYPE html><html><head>"
      "<meta charset='UTF-8'>"
      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
      "<title>Control Climático</title>"
      "<style>"
      "body {font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #0a0e17; color: #fff; position: relative; overflow: hidden; height: 100vh;}"
      ".container {max-width: 500px; margin: 0 auto; padding: 20px; background: rgba(255, 255, 255, 0.95); border-radius: 10px; box-shadow: 0 2px 20px rgba(0,0,0,0.3); position: relative; z-index: 10; color: #333;}"
      "h1 {color: #2c3e50; text-align: center; margin-top: 0;}"
      ".status {background-color: #f8f9fa; padding: 15px; border-radius: 5px; margin-bottom: 20px; border: 1px solid #eee;}"
      ".status-item {display: flex; justify-content: space-between; margin-bottom: 10px;}"
      ".btn-container {display: grid; grid-template-columns: 1fr 1fr; gap: 10px;}"
      ".btn {padding: 12px; border: none; border-radius: 5px; color: white; font-weight: bold; cursor: pointer; transition: all 0.3s; text-align: center; text-decoration: none; display: block;}"
      ".btn-motor-on {background-color: #28a745;}"
      ".btn-motor-off {background-color: #dc3545;}"
      ".btn-servo-on {background-color: #007bff;}"
      ".btn-servo-off {background-color: #6c757d;}"
      ".btn:hover {opacity: 0.9; transform: translateY(-2px); box-shadow: 0 2px 5px rgba(0,0,0,0.2);}"
      ".star {position: absolute; background-color: #fff; border-radius: 50%; animation: twinkle var(--duration) infinite ease-in-out;}"
      "@keyframes twinkle {"
      "  0%, 100% {opacity: 0.2;}"
      "  50% {opacity: 1;}"
      "}"
      "</style>"
      "<script>"
      "function updateStatus() {"
      "  fetch('/status').then(r=>r.json()).then(data=>{"
      "    document.getElementById('temp-value').innerHTML = data.temperature + ' &deg;C';"
      "    document.getElementById('hum-value').innerText = data.humidity + '%';"
      "    document.getElementById('motor-status').innerText = data.motor;"
      "    document.getElementById('servo-status').innerText = data.servo;"
      "    document.getElementById('motor-status').style.color = data.motor === 'ON' ? '#28a745' : '#dc3545';"
      "    document.getElementById('servo-status').style.color = data.servo === 'ON' ? '#007bff' : '#6c757d';"
      "  });"
      "}"
      "function createStars() {"
      "  const body = document.body;"
      "  const starCount = 100;"
      "  "
      "  for(let i = 0; i < starCount; i++) {"
      "    const star = document.createElement('div');"
      "    star.className = 'star';"
      "    star.style.width = (Math.random() * 3 + 1) + 'px';"
      "    star.style.height = star.style.width;"
      "    star.style.left = Math.random() * 100 + '%';"
      "    star.style.top = Math.random() * 100 + '%';"
      "    star.style.setProperty('--duration', (Math.random() * 5 + 3) + 's');"
      "    star.style.animationDelay = (Math.random() * 5) + 's';"
      "    body.appendChild(star);"
      "  }"
      "}"
      "setInterval(updateStatus, 2000);"
      "window.onload = function() {"
      "  createStars();"
      "  updateStatus();"
      "}"
      "</script>"
      "</head>"
      "<body>"
      "<div class='container'>"
      "<h1>Control Climático</h1>"
      "<div class='status'>"
      "<div class='status-item'><span>Temperatura:</span><span id='temp-value'>0 &deg;C</span></div>"
      "<div class='status-item'><span>Humedad:</span><span id='hum-value'>0%</span></div>"
      "<div class='status-item'><span>Motor Ventilador:</span><span id='motor-status'>OFF</span></div>"
      "<div class='status-item'><span>Limpiaparabrisas:</span><span id='servo-status'>OFF</span></div>"
      "</div>"
      "<div class='btn-container'>"
      "<a href='/motor/on' class='btn btn-motor-on'>Encender Motor</a>"
      "<a href='/motor/off' class='btn btn-motor-off'>Apagar Motor</a>"
      "<a href='/servo/on' class='btn btn-servo-on'>Activar Limpiaparabrisas</a>"
      "<a href='/servo/off' class='btn btn-servo-off'>Desactivar Limpiaparabrisas</a>"
      "</div>"
      "</div>"
      "</body></html>"
    );
    server.send(200, "text/html", html);
  });

  server.on("/motor/on", []() {
    motorManual = true;
    controlarMotor(true);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/motor/off", []() {
    motorManual = false;
    controlarMotor(false);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/servo/on", []() {
    servManual = true;
    controlarServo(true);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/servo/off", []() {
    servManual = false;
    controlarServo(false);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/status", []() {
    TempAndHumidity data;
    data.temperature = 21.20;
    data.humidity = 65.00;
    
    String json = "{";
    json += "\"temperature\":" + String(data.temperature, 1) + ",";
    json += "\"humidity\":" + String(data.humidity, 1) + ",";
    json += "\"motor\":\"" + String(estadoMotor ? "ON" : "OFF") + "\",";
    json += "\"servo\":\"" + String(estadoServo ? "ON" : "OFF") + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}