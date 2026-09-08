#include <Arduino.h>


const int ledPin = 5;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  Serial.println("Praktikum 1 - Digital Output Dimulai");
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(ledPin, HIGH);
  Serial.println("LED Menyala");
  delay(1000);

  digitalWrite(ledPin, LOW);
  Serial.println("LED Mati");
  delay(1000);
}

