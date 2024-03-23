#include<WiFi.h>
#include<PubSubClient.h>
#include "DHT.h"
#include<ArduinoJson.h>

#define DHTpin 2
#define DHTType DHT11
DHT dht(DHTpin, DHTType);

const int led[2] = {4, 5};
const int btn[2] = {9, 10};
boolean updateState = 0;
unsigned long timenow = millis();

const char* ssid = "iphone";
const char* password = "hoilamgi";

const char*  mqtt_server = "7d6e368f03d04eadac161b0357f0225c.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "tuong";
const char* mqtt_password = "tuong194";

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// ham ngat button
ICACHE_RAM_ATTR void handleBtn() {
  if (millis() - timenow > 500) {
    for (int i = 0; i < 2; i++) {
      if (digitalRead(btn[i]) == LOW) {
        digitalWrite(led[i], !digitalRead(led[i]));
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
    digitalWrite(led[0], p);
    Serial.println("led1: " + String(p));
  }
  if (obj.containsKey("led2")) {
    boolean p = obj["led2"];
    digitalWrite(led[1], p);
    Serial.println("led2: " + String(p));
  }
  updateState = 1;
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

  for (int i = 0; i < 2; i++) {
    pinMode(led[i], OUTPUT);
    pinMode(btn[i], INPUT_PULLUP);
    attachInterrupt(btn[i], handleBtn, FALLING);
  }

  //mqtt
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
  mqtt_client.subscribe("esp/sub");
  mqtt_client.loop();

  if (millis() - timenow > 1000) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    DynamicJsonDocument doc(1024);
    doc["humidity"] = h;
    doc["temperature"] = t;
    char mqtt_message[128];
    serializeJson(doc, mqtt_message);
    publishMsg("esp/dht11", mqtt_message, true);
    timenow = millis();
  }
//gui trang thi cua led
  if (updateState == 1) {
    DynamicJsonDocument doc(1024);
    doc["led1"] = digitalRead(led[0]);
    doc["led2"] = digitalRead(led[1]);
    char mqtt_message[128];
    serializeJson(doc, mqtt_message);
    publishMsg("esp/led", mqtt_message, true);
    updateState = 0;
  }

}
