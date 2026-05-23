#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include "MAX30105.h" 
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------- 1. إعدادات الـ BLE ----------
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// ---------- 2. تعريف الحساسات ----------
const int TEMP_PIN = 4;    
const int ECG_PIN  = 35;   

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
MAX30105 particleSensor;

uint32_t current_packet_id = 0;
unsigned long lastSend = 0;

// فئة للتحكم في حالة الاتصال
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      // إعادة البث للبحث عند الفصل
      BLEDevice::startAdvertising();
    }
};

void setup() {
  Serial.begin(115200);

  // 1. تشغيل الحساسات
  sensors.begin(); 
  Wire.begin(21, 22); 
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 NOT FOUND!");
  } else {
    particleSensor.setup(); 
  }

  // 2. إعداد الـ BLE
  BLEDevice::init("Wheelchair_ESP32"); // الاسم اللي هيظهر في الأيفون
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("Waiting for iPhone connection...");
}

void loop() {
  if (millis() - lastSend > 5000) { 
    lastSend = millis();
    current_packet_id++; 

    // قراءة الحرارة
    sensors.requestTemperatures(); 
    float temperature = sensors.getTempCByIndex(0);

    // قراءة الـ ECG
    int ecgValue = analogRead(ECG_PIN);

    // قراءة النبض
    long irValue = particleSensor.getIR(); 
    int heartRate = (irValue > 50000) ? map(irValue, 50000, 150000, 60, 100) : 0;

    // بناء الـ JSON
    String payload = "{";
    payload += "\"HR\":" + String(heartRate) + ",";
    payload += "\"T\":" + String(temperature, 1) + ",";
    payload += "\"ECG\":" + String(ecgValue) + ",";
    payload += "\"ID\":" + String(current_packet_id);
    payload += "}";

    Serial.println("Sending: " + payload);

    if (deviceConnected) {
      pCharacteristic->setValue(payload.c_str());
      pCharacteristic->notify(); // إرسال تحديث للأيفون
    }
  }
}