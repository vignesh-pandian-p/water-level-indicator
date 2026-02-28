#include <WiFi.h>
#include <esp_now.h>
#include <HardwareSerial.h>

/* ===================== LoRa UART ===================== */
#define LORA_RX 16
#define LORA_TX 17
#define BOOT_BTN 0
HardwareSerial LoRaSerial(1);

/* ===================== Ultrasonic ===================== */
#define TRIG_PIN 5
#define ECHO_PIN 18

/* ===================== Moving Average ===================== */
#define FILTER_SIZE 5
float distanceBuffer[FILTER_SIZE];
int bufferIndex = 0;

/* ===================== Motor ===================== */
bool motorState = false;

/* ===================== ESP-NOW Data ===================== */
typedef struct struct_message {
  float temperature;
  float turbidityVoltage;
  int waterStatus;
} struct_message;

struct_message receivedData;
bool newWaterData = false;

/* ===================== Ultrasonic Function ===================== */
float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = 0;
  for (int i = 0; i < 3 && duration == 0; i++) {
    duration = pulseIn(ECHO_PIN, HIGH, 60000);
  }

  if (duration == 0) return -1;
  return (duration * 0.0343) / 2.0;
}

/* ===================== ESP-NOW Receive Callback (UPDATED) ===================== */
void OnDataRecv(const esp_now_recv_info *info,
                const uint8_t *incomingData,
                int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  newWaterData = true;

  Serial.println("ESP-NOW Data Received");
}

/* ===================== Setup ===================== */
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BOOT_BTN, INPUT_PULLUP);
  digitalWrite(TRIG_PIN, LOW);

  /* -------- ESP-NOW Init -------- */
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  /* -------- LoRa Init -------- */
  LoRaSerial.begin(115200, SERIAL_8N1, LORA_RX, LORA_TX);
  Serial.println("ESP-NOW + Ultrasonic + LoRa READY");
}

/* ===================== Loop ===================== */
void loop() {

  /* -------- Ultrasonic Reading -------- */
  float distance_cm = readDistanceCM();

  if (distance_cm > 0) {
    distanceBuffer[bufferIndex] = distance_cm;
    bufferIndex = (bufferIndex + 1) % FILTER_SIZE;

    float avg = 0;
    for (int i = 0; i < FILTER_SIZE; i++) avg += distanceBuffer[i];
    avg /= FILTER_SIZE;

    Serial.print("Distance(avg): ");
    Serial.print(avg, 1);
    Serial.println(" cm");

    /* -------- Motor Logic -------- */
    if (avg > 79.0 && !motorState) {
      motorState = true;
      Serial.println("Motor ON (Water Low)");
    }
    else if (avg < 50.0 && motorState) {
      motorState = false;
      Serial.println("Motor OFF (Tank Full)");
    }

    /* -------- Send via LoRa when ESP-NOW data arrives -------- */
    if (newWaterData) {
      String msg =
        "DIST:" + String(avg, 1) +
        ",MOTOR:" + String(motorState ? "ON" : "OFF") +
        ",TEMP:" + String(receivedData.temperature, 1) +
        ",TURB:" + String(receivedData.turbidityVoltage, 2) +
        ",STATUS:" + String(receivedData.waterStatus);

      Serial.println("Sending via LoRa:");
      Serial.println(msg);

      LoRaSerial.print("AT+SEND=0,");
      LoRaSerial.print(msg.length());
      LoRaSerial.print(",");
      LoRaSerial.println(msg);

      newWaterData = false;
    }
  }
  else {
    Serial.println("Ultrasonic: No Echo");
  }

  /* -------- Manual Motor Toggle -------- */
  if (digitalRead(BOOT_BTN) == LOW) {
    delay(50);
    motorState = !motorState;
    Serial.println(motorState ? "Motor Manually ON" : "Motor Manually OFF");
    delay(500);
  }

  delay(500);
}