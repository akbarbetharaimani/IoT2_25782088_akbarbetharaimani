#include <Arduino.h>

const int buttonPin = 4;
const int ledPin = 5;

int lastButtonReading = LOW;
int stableButtonState = LOW;
bool ledState = false;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  Serial.println("Praktikum 1 - Digital Output Dimulai");
}

void loop() {
  // put your main code here, to run repeatedly:
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != stableButtonState) {
      stableButtonState = reading;

      if (stableButtonState == HIGH) {
        ledState = !ledState;
        digitalWrite(ledPin, ledState ? HIGH : LOW);
        Serial.print("LED ");
        Serial.println(ledState ? "ON" : "OFF");
      }
    }
  }

  lastButtonReading = reading;
}