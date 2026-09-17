#define BUZZER_PIN 25
#define BUTTON_PIN 26

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Button pressed - alert cancelled");
  } else {
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("Waiting for button");
  }

  delay(200);
}