#define BLYNK_TEMPLATE_ID "TMPL6gesrj4Zb"
#define BLYNK_TEMPLATE_NAME "ESP32 Fish Feeder"
#define BLYNK_FIRMWARE_VERSION "0.1.0"  
#define BLYNK_PRINT Serial
#define APP_DEBUG
#define USE_ESP32_DEV_MODULE

#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP32Servo.h>
#include "BlynkEdgent.h"

Servo myServo;
const int servoPin = 18;

const int echoPin = 2;  
const int trigPin = 4;  
long duration;
float distance;

const char* ntpServer = "time.cloudflare.com"; 
const long gmtOffset_sec = 7 * 3600;    
const int daylightOffset_sec = 0;       

WiFiUDP udp;
NTPClient timeClient(udp, ntpServer, gmtOffset_sec, 60000); 

String schedules[10];
int rotateCounts[10];

int scheduleIndex = 0;

void setup() {
  Serial.begin(9600);
  BlynkEdgent.begin();
  
  myServo.attach(servoPin);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  timeClient.begin();
  Serial.println("Waiting for NTP time...");

  getCurrentTime();
}

void loop() {
  BlynkEdgent.run();
  timeClient.update();  // Continuously update NTP time
  
  const int currentTimeInSeconds = getCurrentTime();

  for (int i = 0; i < 10; i++) {
    if (schedules[i] != "") {
      int scheduledTimeInSeconds = schedules[i].toInt();
      
      // If the current time matches the schedule, trigger the servo
      if (currentTimeInSeconds >= scheduledTimeInSeconds && currentTimeInSeconds < (scheduledTimeInSeconds + 5)) {
        int rotateCountValue = rotateCounts[i] ? rotateCounts[i] : 1;
        Serial.print(rotateCounts[i]);
        rotateServo(rotateCountValue);
      }
    }
  }

  triggerUltraSonic();
  delay(1000); 
}

BLYNK_WRITE(V0) {
  const String second = param.asString();
  schedules[scheduleIndex] = second;

  updateTimeDisplay(second);

  for (int i = 0; i < 10; i++) {
    Serial.println("Index " + String(i) + ": " + schedules[i]);
  }
}

BLYNK_WRITE(V1) {
  scheduleIndex = param.asInt();

  const String second = schedules[scheduleIndex];
  updateTimeDisplay(second);
}

void updateTimeDisplay(String data) {
  const int second = data.toInt();

  const int hours = (second / 3600) % 24;
  const int minutes = (second / 60) % 60;

  char timeBuffer[6];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", hours, minutes);
  String formattedTime = String(timeBuffer);

  Blynk.virtualWrite(V2, formattedTime);
}

BLYNK_WRITE(V4) {
  int rotateCount = param.asInt();
  rotateCounts[scheduleIndex] = rotateCount;

  for (int i = 0; i < 10; i++) {
    Serial.println("Index " + String(i) + ": " + rotateCounts[i]);
  }
}

BLYNK_WRITE(V6) {
  int isOn = param.asInt();

  if(isOn) {
    rotateServo(1);
  }
}

void triggerUltraSonic() {
  // Trigger the ultrasonic sensor
  digitalWrite(trigPin, LOW);
  delay(1000); // Short delay for stable LOW
  digitalWrite(trigPin, HIGH);
  delay(1000); // Send 10us HIGH pulse
  digitalWrite(trigPin, LOW);

  // Read the echo pin and calculate distance
  duration = pulseIn(echoPin, HIGH);
  distance = duration / 58.2; // Convert duration to cm

  int threshold = 10; // Threshold for distance in cm
  int percentage;

  if (distance >= threshold || distance == 0) { // Handle out-of-range values
    percentage = 0;
  } else {
    percentage = 100 - ((distance / threshold) * 100);
    if (percentage > 100) percentage = 100; // Cap percentage at 100
    if (percentage < 0) percentage = 0;     // Avoid negative percentages
  }

  // Write the calculated percentage to Blynk
  Blynk.virtualWrite(V3, percentage);
}

void rotateServo(int count) {
  Blynk.virtualWrite(V5, count);

  for (int i = 0; i < count; i++) {
    myServo.write(0);   
    delay(1000);       
    myServo.write(180); 
    delay(1000);     
    myServo.write(0);   
    delay(1000);       
  }
}

int getCurrentTime() {
  // Fetch current time from NTP client
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  int currentSecond = timeClient.getSeconds();
  int totalSeconds = (currentHour * 3600) + (currentMinute * 60) + currentSecond;

  // Print current time to Serial Monitor
  Serial.print("Current Time: ");
  Serial.print(currentHour);
  Serial.print(":");
  Serial.print(currentMinute);
  Serial.print(":");
  Serial.println(currentSecond);

  return totalSeconds;
}