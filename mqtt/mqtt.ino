#include<WiFi.h>
#include <WiFiClientSecure.h>
#include<PubSubClient.h>
#include "DHT.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include<ArduinoJson.h>

DynamicJsonDocument JSdoc(1024);
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTpin 23  // dht11
#define DHTType DHT11
DHT dht(DHTpin, DHTType);

const int out[3] = {2, 4, 15}; // led1,led2,quat
const int btn[3] = {5, 18, 19};

#define coi 13
#define gas 34 // chan -

//cam bien mua va gian phoi
#define mua 14
#define ctht1 27
#define ctht2 26
#define in1 25
#define in2 33


uint8_t checkRun;

String PubString;

float humi;  // do am
float tempC; // nhiet do C
boolean updateState = 0; // update output
unsigned long timenow = millis();

// connect to WIFI
const char* ssid = "XOM TRO";
const char* password = "deptraicogisai";

//MQTT HiveMQ
const char*  mqtt_server = "fdac775f27af4637b3322633ca2202c1.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "tuong";
const char* mqtt_password = "tuong194";

//WiFiClient espClient;
WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);



// ham ngat button
ICACHE_RAM_ATTR void handleBtn() {
  if (millis() - timenow > 200) {
    for (int i = 0; i < 3; i++) {
      if (digitalRead(btn[i]) == LOW) {
        digitalWrite(out[i], !digitalRead(out[i]));
      }
    }
    updateState = 1;
    timenow = millis();
  }
}

// connect to wifi
void setup_wifi() {
  Serial.print("Connecting to wifi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
}

/*nhan du lieu tu mqtt*/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("RX from ");
  Serial.print(topic);
  String RxMsg = "";
  for (int i = 0; i < length; i++) {
    RxMsg += (char)payload[i];
  }
  Serial.println(RxMsg);


  DynamicJsonDocument doc(100);
  DeserializationError error = deserializeJson(doc, RxMsg);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  JsonObject obj = doc.as<JsonObject>();
  if (obj.containsKey("led1")) { // neu co doi tuong led1
    boolean p = obj["led1"];
    digitalWrite(out[0], p);
  }
  if (obj.containsKey("led2")) {
    boolean p = obj["led2"];
    digitalWrite(out[1], p);

  }
  if (obj.containsKey("quat")) { // neu co doi tuong led1
    boolean p = obj["quat"];
    digitalWrite(out[2], p);
  }
  if (obj.containsKey("auto")) {
    updateState = obj["auto"];
  }
  if (obj.containsKey("run")) {
    if (obj["run"] == 1) { // tien
      checkRun = 1;
    } else if (obj["run"] == 0) {
      checkRun = 0;
    } else if (obj["run"] == 2) {
      dung();
    }
  }
}

/*gui du lieu len mqtt*/
void publishMsg(const char* topic, String payload, boolean retained) {
  if (mqtt_client.publish(topic, payload.c_str(), true)) {
    Serial.println("message publish [" + String(topic) + "] : " + payload);
  }
}
void setup() {
  Serial.begin(9600);
  setup_wifi();
  dht.begin();

  for (int i = 0; i < 3; i++) {
    pinMode(out[i], OUTPUT);
    pinMode(btn[i], INPUT_PULLUP);
    attachInterrupt(btn[i], handleBtn, FALLING);
  }
  pinMode(coi, OUTPUT);

  pinMode(ctht1, INPUT);
  pinMode(ctht2, INPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

 // pinMode(gas, INPUT);
  pinMode(mua, INPUT);
  lcd.init();
  lcd.backlight();
 
  dht.begin();

  espClient.setInsecure();
  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(callback);// nhan du lieu tra ve tu mqtt

  while (!mqtt_client.connected()) {
    Serial.print("wating mqtt connect ...");
    String clientID = "ESPClient-";
    clientID += String(random(0xffff), HEX);
    if (mqtt_client.connect(clientID.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("MQTT connected!!!");
    } else {
      Serial.println("MQTT failed");
    }
  }
}

void loop() {

  mqtt_client.loop();
  mqtt_client.subscribe("esp/sub");

  gasf();
  if (updateState) { // bat che do auto
    rain();
    checkRun =2;
  } else if (!updateState) {
    if (checkRun == 1) {
      tien();
      JSdoc["rain"] = 0;
      if (digitalRead(ctht2) == 0 && digitalRead(ctht1) == 1) {
        dung();
      }
    } else if (checkRun == 0) {
      lui();
      JSdoc["rain"] = 1;
      if (digitalRead(ctht1) == 0 && digitalRead(ctht2) == 1) {
        dung();
      }
    }
  }

  temp();
  ReadStatus();

  if (millis() - timenow >= 3000) {
    serializeJson(JSdoc, PubString);
    publishMsg("esp/pub", PubString , true);
    timenow = millis();
    PubString = "";
  }
}

void gasf() {
  int giatri = analogRead(gas);
  if (giatri >= 2800 ) {
    digitalWrite(coi, HIGH);
    JSdoc["gas"] = 1; // bao dong
  } else {
    digitalWrite(coi, LOW);
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
  humi = dht.readHumidity();
  tempC = dht.readTemperature();

  JSdoc["tempC"] = String(tempC);
  JSdoc["humi"] = String(humi);

  lcd.setCursor(0, 0);
  lcd.print("NHIET DO ");
  lcd.setCursor(9, 0);
  lcd.print(tempC);

  lcd.setCursor(0, 1);
  lcd.print("DO AM ");
  lcd.setCursor(9, 1);
  lcd.print(humi);
}
void ReadStatus() {
  JSdoc["led1"] = digitalRead(out[0]);
  JSdoc["led2"] = digitalRead(out[1]);
  JSdoc["quat"] = digitalRead(out[2]);
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
