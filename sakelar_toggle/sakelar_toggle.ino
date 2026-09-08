
const int buttonPin = 4;  
const int ledPin = 5;    


bool ledState = false;        
int lastButtonReading = LOW;   
int stableButtonState = LOW;   

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {
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

        Serial.print("Tombol ditekan -> LED sekarang: ");
        Serial.println(ledState ? "ON (menyala tetap)" : "OFF (mati tetap)");
      }
    }
  }

  lastButtonReading = reading;
}
