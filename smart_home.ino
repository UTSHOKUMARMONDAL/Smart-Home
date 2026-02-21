#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include <Keypad.h>
#include <Password.h>

#define SENSOR_PIN 2
#define BUZZER_PIN 3
#define RELAY_PIN 4
#define SPRINKLER_START_DELAY 3000
#define SPRINKLER_ON_TIME 5000

#define PASSWORD_BUZZER 8
#define SERVO_PIN 9

SoftwareSerial Bluetooth(10, 11);
Servo myServo;
Servo doorServo;

char Data;
unsigned long previousTime = millis();
bool alertSent = false;

#define sim800l Serial

Password password = Password("0123");
byte maxPasswordLength = 6;
byte currentPasswordLength = 0;
bool value = true;

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'D', 'C', 'B', 'A'},
  {'#', '9', '6', '3'},
  {'0', '8', '5', '2'},
  {'*', '7', '4', '1'},
};

byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {A4, A5, 12, 13};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Bluetooth.begin(9600);
  Serial.begin(9600);

  myServo.attach(SERVO_PIN);

  myServo.write(150);
 


  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PASSWORD_BUZZER, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH);
  analogWrite(BUZZER_PIN, 0);

  sim800l.begin(9600);
  delay(1000);
  sim800l.println("AT");
  delay(1000);
  sim800l.println("AT+CMGF=1");
  delay(1000);
}

void loop() {
  handleBluetooth();
  handleFireSensor();
  handleKeypad();
}

void handleBluetooth() {
  if (Bluetooth.available()) {
    Data = Bluetooth.read();

    if (Data == '5') digitalWrite(5, LOW);
    if (Data == '1') digitalWrite(5, HIGH);

    if (Data == '6') digitalWrite(6, LOW);
    if (Data == '2') digitalWrite(6, HIGH);

    if (Data == '7') digitalWrite(7, LOW);
    if (Data == '3') digitalWrite(7, HIGH);

    if (Data == '9') {
      digitalWrite(5, LOW);
      digitalWrite(6, LOW);
      digitalWrite(7, LOW);
    }

    if (Data == '0') {
      digitalWrite(5, HIGH);
      digitalWrite(6, HIGH);
      digitalWrite(7, HIGH);
    }

    if (Data == 'a') myServo.write(50);
    if (Data == 'b') myServo.write(150);
  }
}
void handleFireSensor() {
  int sensorValue = digitalRead(SENSOR_PIN);

  if (sensorValue == LOW) {
    analogWrite(BUZZER_PIN, 50);

    if (millis() - previousTime > SPRINKLER_START_DELAY) {
    
      digitalWrite(RELAY_PIN, LOW);

      
      if (!alertSent) {
        sendSMS("+8801865884889", "🔥 Fire detected! Sprinkler activated. Please respond immediately.");
        delay(1000);
        makeCall("+8801865884889");
        alertSent = true;
      }

      delay(SPRINKLER_ON_TIME);  
    }
  } else {
    analogWrite(BUZZER_PIN, 0);
    digitalWrite(RELAY_PIN, HIGH); 
    previousTime = millis();       
    alertSent = false;             
  }
}


void handleKeypad() {
  char key = keypad.getKey();

  if (key != NO_KEY) {
    delay(60);
    if (key == 'C') {
      resetPassword();
    } else if (key == 'D') {
      if (value) {
        doorlocked();
        value = false;
      } else {
        dooropen();
        value = true;
      }
    } else {
      processNumberKey(key);
    }
  }
}

void processNumberKey(char key) {
  currentPasswordLength++;
  password.append(key);

  if (currentPasswordLength == maxPasswordLength) {
    if (value) {
      dooropen();
      value = false;
    } else {
      doorlocked();
      value = true;
    }
  }
}

void dooropen() {
  if (password.evaluate()) {
    correctBeep();
    myServo.write(50);
  } else {
    wrongBeep();
  }
  resetPassword();
}

void doorlocked() {
  if (password.evaluate()) {
    correctBeep();
    myServo.write(150);
  } else {
    wrongBeep();
  }
  resetPassword();
}

void resetPassword() {
  password.reset();
  currentPasswordLength = 0;
}

void correctBeep() {
  digitalWrite(PASSWORD_BUZZER, HIGH);
  delay(200);
  digitalWrite(PASSWORD_BUZZER, LOW);
  delay(100);
}

void wrongBeep() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(PASSWORD_BUZZER, HIGH);
    delay(150);
    digitalWrite(PASSWORD_BUZZER, LOW);
    delay(150);
  }
}

void sendSMS(String number, String message) {
  sim800l.print("AT+CMGS=\"");
  sim800l.print(number);
  sim800l.println("\"");
  delay(1000);
  sim800l.print(message);
  delay(500);
  sim800l.write(26);
  delay(5000);
}

void makeCall(String number) {
  sim800l.print("ATD");
  sim800l.print(number);
  sim800l.println(";");
  delay(20000);
  sim800l.println("ATH");
}
