#include<ESP8266WiFi.h>
#include<PubSubClient.h>
#include<ArduinoJson.h>
#include <WiFiClientSecure.h>
#include<ArduinoJson.h>

String inputString="";
unsigned long timenow = millis();

const char* ssid = "XOM TRO";
const char* password = "deptraicogisai";

const char*  mqtt_server = "7d6e368f03d04eadac161b0357f0225c.s1.eu.hivemq.cloud";
//fdac775f27af4637b3322633ca2202c1
const int mqtt_port = 8883;
const char* mqtt_username = "tuong";
const char* mqtt_password = "tuong194";

WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);


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

  String RxMsg = "";
  for (int i = 0; i < length; i++) {
    RxMsg += (char)payload[i];
  }
  Serial.print(RxMsg);
}

/*gui du lieu len mqtt*/
void publishMsg(const char* topic, String payload, boolean retained) {
  if (mqtt_client.publish(topic, payload.c_str(), true)) {
    //Serial.println("message publish [" + String(topic) + "] : " + payload);
  }
}
void setup() {
  Serial.begin(9600);
  setup_wifi();

  espClient.setInsecure(); // bỏ qua xác nhận chứng chỉ SSL/TLS
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

  if(Serial.available()){
    inputString = Serial.readString();
     publishMsg("esp/dht11", inputString, true);
  }

}
