#include <Wire.h>
#include"DHT.h"
#include <LiquidCrystal_I2C.h>
#include<ArduinoJson.h>

unsigned long t1 = 0;

DynamicJsonDocument JSdoc(1024);
String inputString;


#define coi 22
#define gas 24

#define mua 26
#define ctht1 28
#define ctht2 30

//keo dan phoi
#define in1 31 //L298N
#define in2 32

const int out[3] = {33, 34, 35}; //led1, led2, quat
const int btn[3] = {37, 38, 39}; //led1, led2, quat

#define DHT11Pin 36
#define DHTType DHT11
DHT HT(DHT11Pin, DHTType);

float humi;  // do am
float tempC; // nhiet do C
float tempF; // do F

bool state = HIGH;



void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  pinMode(coi, OUTPUT);

  pinMode(ctht1, INPUT);
  pinMode(ctht2, INPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  pinMode(gas, INPUT);
  pinMode(mua, INPUT);
  for (int i = 0; i < 3; i++) {
    pinMode(out[i], OUTPUT);
    pinMode(btn[i], INPUT_PULLUP);
    //attachInterrupt(digitalPinToInterrupt(btn[i]), handleBtn, FALLING);
  }
  HT.begin();

}

void loop() {
  for (int i = 0; i < 3; i++) {
    pinMode(out[i], OUTPUT);
    pinMode(btn[i], INPUT_PULLUP);
    //attachInterrupt(digitalPinToInterrupt(btn[i]), handleBtn, FALLING);
  }




    gasf();
    rain();
    temp();
    ReadStatus();

    sendData();
    readData();

}

//hàm ngắt khi nhấn nut button
void handleBtn() {
  if (millis() - t1 > 500) {
    for (int i = 0; i < 2; i++) {
      if (digitalRead(btn[i]) == LOW) {
        digitalWrite(out[i], !digitalRead(out[i]));
      }
    }
    t1 = millis();
  }
}

void gasf() {
  int giatri = digitalRead(gas);
  if (giatri == 0) {
    tone(coi, 500); // tone(pin, tần số Hz)
    JSdoc["gas"] = 1;
  } else {
    noTone(coi);
    JSdoc["gas"] = 0;
  }
}
void rain() {
  int giatri2 = digitalRead(mua);
  if (giatri2 == 0) { // mưa, kéo giàn phơi vào
    lui();
    JSdoc["rain"] = 1;
    if (digitalRead(ctht1) == 0 && digitalRead(ctht2) == 1) {
      dung();
    }

  } else { // tạnh thì kéo giàn phơi ra
    tien();
    JSdoc["rain"] = 0;
    if (digitalRead(ctht2) == 0 && digitalRead(ctht1) == 1) {
      dung();
    }
  }

}

void temp() {
  humi = HT.readHumidity();
  tempC = HT.readTemperature();

  JSdoc["temp"] = String(tempC);
  JSdoc["humi"] = String(humi);
}
void ReadStatus() {
  JSdoc["led1"] = digitalRead(out[0]);
  JSdoc["led2"] = digitalRead(out[1]);
  //JSdoc["quat"] = digitalRead(out[2]);
}

void tien() {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
}
void lui() {
  digitalWrite(in2, HIGH);
  digitalWrite(in1, LOW);

}
void dung() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}
void sendData() {
  if (millis() - t1 > 2000) {
    serializeJson(JSdoc, Serial2);
    serializeJson(JSdoc, Serial);
    Serial.println();
    t1 = millis();
  }
}

void readData() {
  if (Serial2.available()) {
    inputString = Serial2.readString();
    if (inputString == "{\"led1\":1") {
      digitalWrite(out[0], 1);
      Serial.println("led1: " + String(1));
    } else if (inputString == "{\"led1\":0") {
      digitalWrite(out[0], 0);
      Serial.println("led1: " + String(0));
    } else if (inputString == "{\"led2\":1") {
      digitalWrite(out[1], 1);
      Serial.println("led2: " + String(1));
    } else if (inputString == "{\"led2\":0") {
      digitalWrite(out[1], 0);
      Serial.println("led2: " + String(0));
    } else if (inputString == "{\"quat\":1") {
      digitalWrite(out[2], 1);
      Serial.println("quat: " + String(1));
    } else if (inputString == "{\"quat\":0") {
      digitalWrite(out[2], 0);
      Serial.println("quat: " + String(0));
    }

  }
}

void readData2() {
  if (Serial2.available()) {
    while (Serial2.available()) {
      inputString = Serial2.readString();
    }
    DynamicJsonDocument doc(100);
    DeserializationError error = deserializeJson(doc, inputString);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }
    JsonObject obj = doc.as<JsonObject>();
    if (obj.containsKey("led1")) {
      boolean p = obj["led1"];
      digitalWrite(out[0], p);
      Serial.println("led1: " + String(p));
    }
    if (obj.containsKey("led2")) {
      boolean p = obj["led2"];
      digitalWrite(out[1], p);
      Serial.println("led2: " + String(p));
    }
    if (obj.containsKey("quat")) {
      boolean p = obj["quat"];
      digitalWrite(out[2], p);
      Serial.println("quat: " + String(p));
    }
  }
}
