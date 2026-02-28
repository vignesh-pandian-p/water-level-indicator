#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

/* ================= WIFI ================= */
const char* ssid = "Shiyam";
const char* password = "12345678";

/* ================= FIREBASE ================= */
const char* firebaseHost =
  "https://water-level-indicator-201f4-default-rtdb.firebaseio.com";
const char* firebasePath = "/waterLevel.json";

/* ================= LORA ================= */
#define LORA_RX 16
#define LORA_TX 17
HardwareSerial LoRaSerial(1);

/* ================= RELAYS ================= */
#define RELAY_IN1 25   // Motor ON relay
#define RELAY_IN2 26   // Motor OFF relay

/* ================= OLED ================= */
#define I2C_ADDRESS 0x3C
Adafruit_SH1106G display(128, 64, &Wire, -1);

/* ================= DATA ================= */
float distanceCM = 0;
bool motorState = false;
bool lastMotorState = false;

float waterTemp = 0;
float turbidityVolt = 0;
String waterQuality = "--";

/* ================= RELAY TIMER ================= */
bool relayActive = false;
unsigned long relayStartTime = 0;
const unsigned long RELAY_PULSE_TIME = 2000; // 2 seconds

/* ================= FIREBASE TIMER ================= */
unsigned long lastFirebaseUpdate = 0;
const unsigned long FIREBASE_INTERVAL = 5000;

/* ================= WIFI ================= */
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("WiFi connecting");
  unsigned long t0 = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    Serial.print(".");
    delay(300);
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nWiFi OK" : "\nWiFi FAIL");
}

/* ================= DISPLAY ================= */
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  display.setCursor(0, 0);
  display.println("Water Monitoring");

  display.setCursor(0, 12);
  display.print("Dist: ");
  display.print(distanceCM, 1);
  display.println(" cm");

  display.setCursor(0, 22);
  display.print("Motor: ");
  display.println(motorState ? "ON" : "OFF");

  display.setCursor(0, 32);
  display.print("Temp: ");
  display.print(waterTemp, 1);
  display.println(" C");

  display.setCursor(0, 42);
  display.print("Turb: ");
  display.print(turbidityVolt, 2);
  display.println(" V");

  display.setCursor(0, 52);
  display.print("WQ: ");
  display.println(waterQuality);

  display.display();
}

/* ================= FIREBASE ================= */
void sendFirebase() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(String(firebaseHost) + firebasePath);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["distance"] = distanceCM;
  doc["motorState"] = motorState ? "ON" : "OFF";
  doc["temperature"] = waterTemp;
  doc["turbidity"] = turbidityVolt;
  doc["water_quality"] = waterQuality;

  String payload;
  serializeJson(doc, payload);

  http.PUT(payload);
  http.end();
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Relay setup (ACTIVE-LOW)
  pinMode(RELAY_IN1, OUTPUT);
  pinMode(RELAY_IN2, OUTPUT);
  digitalWrite(RELAY_IN1, HIGH);
  digitalWrite(RELAY_IN2, HIGH);

  connectWiFi();

  Wire.begin(21, 22);
  display.begin(I2C_ADDRESS, true);

  LoRaSerial.begin(115200, SERIAL_8N1, LORA_RX, LORA_TX);

  updateDisplay();
  Serial.println("✅ LoRa RX + Firebase + 2-Relay Ready");
}

/* ================= LOOP ================= */
void loop() {

  /* ===== LoRa Receive ===== */
  if (LoRaSerial.available()) {
    String msg = LoRaSerial.readStringUntil('\n');
    msg.trim();

    Serial.println("LoRa RX → " + msg);

    int d = msg.indexOf("DIST:");
    int m = msg.indexOf("MOTOR:");
    int t = msg.indexOf("TEMP:");
    int tb = msg.indexOf("TURB:");
    int wq = msg.indexOf("STATUS:");

    if (d >= 0)
      distanceCM = msg.substring(d + 5, msg.indexOf(",", d)).toFloat();

    bool newMotorState = motorState;
    if (m >= 0)
      newMotorState = msg.substring(m + 6, msg.indexOf(",", m)).startsWith("ON");

    if (t >= 0)
      waterTemp = msg.substring(t + 5, msg.indexOf(",", t)).toFloat();

    if (tb >= 0)
      turbidityVolt = msg.substring(tb + 5, msg.indexOf(",", tb)).toFloat();

    if (wq >= 0) {
      int q = msg.substring(wq + 7).toInt();
      if (q == 0) waterQuality = "CLEAR";
      else if (q == 1) waterQuality = "SLIGHT";
      else waterQuality = "DIRTY";
    }

    /* ===== 2-RELAY PULSE LOGIC ===== */
    if (newMotorState != motorState) {
      motorState = newMotorState;

      if (motorState) {
        Serial.println("Relay1 Pulse (Motor ON)");
        digitalWrite(RELAY_IN1, LOW);
      } else {
        Serial.println("Relay2 Pulse (Motor OFF)");
        digitalWrite(RELAY_IN2, LOW);
      }

      relayStartTime = millis();
      relayActive = true;
      lastMotorState = motorState;
    }

    updateDisplay();

    if (millis() - lastFirebaseUpdate > FIREBASE_INTERVAL) {
      sendFirebase();
      lastFirebaseUpdate = millis();
    }
  }

  /* ===== Relay Pulse Timeout ===== */
  if (relayActive && millis() - relayStartTime >= RELAY_PULSE_TIME) {
    digitalWrite(RELAY_IN1, HIGH);
    digitalWrite(RELAY_IN2, HIGH);
    relayActive = false;
    Serial.println("Relays OFF after pulse");
  }
}