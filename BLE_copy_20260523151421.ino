#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "MAX30105.h" 
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------- 1. الإعدادات ----------
const char* WIFI_SSID   = "SSID";
const char* WIFI_PASS   = "#PASS#";
const char* UBIDOTS_TOKEN = "TOKEN"; 
const char* DEVICE_LABEL = "wheelchair_01"; 

const char* MQTT_SERVER = "industrial.api.ubidots.com";
const uint16_t MQTT_PORT = 1883;

#define TOPIC_PUBLISH "/v1.6/devices/wheelchair_01"
#define TOPIC_SUBSCRIBE "/v1.6/devices/wheelchair_01/packet_id/lv" 

// ---------- 2. تعريف الدبابيس والحساسات ----------
const int TEMP_PIN = 4;    // سلك البيانات بتاع الحساس في بن 4
const int ECG_PIN  = 35;   

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
MAX30105 particleSensor;
WiFiClient espClient;
PubSubClient client(espClient);

volatile bool ackReceived = false;
uint32_t current_packet_id = 0;
unsigned long lastSend = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  ackReceived = true;
}

void setup() {
  Serial.begin(115200);
  
  sensors.begin(); // تشغيل حساس الحرارة DS18B20
  
  Wire.begin(21, 22); 
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 NOT FOUND!");
  } else {
    particleSensor.setup(); 
  }

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (millis() - lastSend > 5000) { 
    lastSend = millis();
    current_packet_id++; 

    // 1. قراءة الحرارة من DS18B20
    sensors.requestTemperatures(); 
    float temperature = sensors.getTempCByIndex(0);

    // 2. قراءة الـ ECG
    int ecgValue = analogRead(ECG_PIN);

    // 3. قراءة النبض
    long irValue = particleSensor.getIR(); 
    int heartRate = (irValue > 50000) ? map(irValue, 50000, 150000, 60, 100) : 0;

    // بناء الـ JSON
    String payload = "{";
    payload += "\"heart_rate\":" + String(heartRate) + ",";
    payload += "\"temperature\":" + String(temperature, 1) + ",";
    payload += "\"ecg\":" + String(ecgValue) + ",";
    payload += "\"packet_id\":" + String(current_packet_id);
    payload += "}";

    Serial.printf("\nSending: %s", payload.c_str());
    if (temperature == -127.00) Serial.println(" -> ERROR: Sensor not found!");
    
    ackReceived = false;
    if (client.publish(TOPIC_PUBLISH, payload.c_str())) {
      unsigned long waitStart = millis();
      while (!ackReceived && (millis() - waitStart < 3000)) {
        client.loop();
        delay(10);
      }
      if (ackReceived) Serial.println(" -> Confirmed!");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(DEVICE_LABEL, UBIDOTS_TOKEN, "")) {
      client.subscribe(TOPIC_SUBSCRIBE);
    } else {
      delay(5000);
    }
  }
}