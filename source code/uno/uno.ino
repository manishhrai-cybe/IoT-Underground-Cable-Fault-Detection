#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

int relayR = 7;
int relayY = 8;
int relayB = 10;
int sensorPin = A0;
int sensorValue = 0;

String getFault(int value) {
  if (value >= 950) return "NF";
  else if (value >= 880) return "8UNIT";
  else if (value >= 820) return "6UNIT";
  else if (value >= 750) return "4UNIT";
  else if (value >= 650) return "2UNIT";
  else return "ERR";
}

// Modified to also return raw ADC via reference
String checkPhase(int relayPin, int &rawOut) {
  digitalWrite(relayPin, LOW);
  delay(300);
  rawOut = analogRead(sensorPin);
  digitalWrite(relayPin, HIGH);
  return getFault(rawOut);
}

void setup() {
  pinMode(relayY, OUTPUT);
  pinMode(relayR, OUTPUT);
  pinMode(relayB, OUTPUT);
  digitalWrite(relayY, HIGH);
  digitalWrite(relayR, HIGH);
  digitalWrite(relayB, HIGH);

  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("UNDERGROUND");
  lcd.setCursor(0, 1);
  lcd.print("CABLE FAULT");
  delay(200);
}

void loop() {
  int rRaw, yRaw, bRaw;

  String rStatus = checkPhase(relayY, rRaw);
  delay(100);
  String yStatus = checkPhase(relayR, yRaw);
  delay(100);
  String bStatus = checkPhase(relayB, bRaw);

  // LCD Display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("R-" + rStatus + " Y-" + yStatus);
  lcd.setCursor(0, 1);
  lcd.print("B-" + bStatus);

  // Add delay before sending to ESP32
  delay(100);
  Serial.println("R," + rStatus + "," + String(rRaw));
  delay(200);
  Serial.println("Y," + yStatus + "," + String(yRaw));
  delay(200);
  Serial.println("B," + bStatus + "," + String(bRaw));

  delay(500);
}