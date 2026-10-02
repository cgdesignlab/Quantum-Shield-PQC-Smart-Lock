#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Servo.h>

// ====== ESP32 as Wi-Fi Access Point ======
const char* ap_ssid     = "PQC_SmartLock";
const char* ap_password = "12345678";

// ===== Kyber Files =====
extern "C" {
  #include "kyber512/api.h"
  #include "kyber512/params.h"
  #include "kyber512/kem.c"
  #include "kyber512/poly.c"
  #include "kyber512/ntt.c"
  #include "kyber512/reduce.c"
  #include "kyber512/polyvec.c"
  #include "kyber512/indcpa.c"
  #include "kyber512/cbd.c"
  #include "kyber512/symmetric-shake.c"
  #include "kyber512/randombytes.c"
  #include "kyber512/fips202.c"
  #include "kyber512/verify.c"
}

// ===== Hardware Pins =====
#define SERVO_PIN 18
#define BUZZER_PIN 25

Servo lockServo;
WiFiServer server(80);

// ===== Continuous Servo Functions =====
// Continuous MG996R = write(90) → STOP

void servoStop() {
  lockServo.write(90);   // stop rotation
}

void rotateClockwise(int duration_ms) {
  lockServo.write(180);  // rotate CW
  delay(duration_ms);
  servoStop();
}

void rotateAntiClockwise(int duration_ms) {
  lockServo.write(0);    // rotate CCW
  delay(duration_ms);
  servoStop();
}

// ===== Door Functions =====
void unlockDoor() {
  Serial.println("UNLOCK");
  rotateClockwise(600);   // adjust time as needed
}

void lockDoor() {
  Serial.println("LOCK");
  rotateAntiClockwise(600);
}

void alarmTrigger() {
  Serial.println("ALARM");
  for (int i = 0; i < 10; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  lockServo.attach(SERVO_PIN);
  servoStop();  // ensure no rotation

  pinMode(BUZZER_PIN, OUTPUT);

  // ---------- Start Wi-Fi AP ----------
  Serial.println("Starting Wi-Fi AP...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(ip);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  String request = client.readStringUntil('\r');
  request.trim();
  Serial.println(request);

  // Handle App Requests
  if (request.indexOf("GET /unlock") != -1) {
    unlockDoor();
  } 
  else if (request.indexOf("GET /lock") != -1) {
    lockDoor();
  } 
  else if (request.indexOf("GET /alarm") != -1) {
    alarmTrigger();
  }

  // Response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("OK");
  client.stop();
}

