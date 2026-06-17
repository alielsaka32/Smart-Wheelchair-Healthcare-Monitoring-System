#include <AFMotor.h>
#include <SoftwareSerial.h>

// إنشاء قناة سيريال للبلوتوث على البنّين 10 (RX) و 11 (TX)
SoftwareSerial BT(10, 11); 

// المواتير على درايفر L293D Shield
AF_DCMotor m1(1), m2(2), m3(3), m4(4);

void setup() {
  Serial.begin(9600);   // للـ Serial Monitor
  BT.begin(9600);       // للبلوتوث HC-05
  Serial.println("🚗 L293D Shield + Bluetooth Ready on pins 10 & 11!");
}

void moveForward() {
  m1.setSpeed(200); m2.setSpeed(200); m3.setSpeed(200); m4.setSpeed(200);
  m1.run(FORWARD); m2.run(FORWARD); m3.run(FORWARD); m4.run(FORWARD);
}

void moveBackward() {
  m1.setSpeed(200); m2.setSpeed(200); m3.setSpeed(200); m4.setSpeed(200);
  m1.run(BACKWARD); m2.run(BACKWARD); m3.run(BACKWARD); m4.run(BACKWARD);
}

void stopMoving() {
  m1.run(RELEASE); m2.run(RELEASE); m3.run(RELEASE); m4.run(RELEASE);
}

void turnLeft() {
  m1.setSpeed(200); m2.setSpeed(200); m3.setSpeed(200); m4.setSpeed(200);
  m1.run(BACKWARD); m3.run(BACKWARD); m2.run(FORWARD); m4.run(FORWARD);
}

void turnRight() {
  m1.setSpeed(200); m2.setSpeed(200); m3.setSpeed(200); m4.setSpeed(200);
  m1.run(FORWARD); m3.run(FORWARD); m2.run(BACKWARD); m4.run(BACKWARD);
}

void loop() {
  // استقبال البيانات من البايثون عبر البلوتوث
  if (BT.available()) {
    String data = BT.readStringUntil('\n');
    data.trim();

    if (data.length() > 0) {
      Serial.print("📩 Received via Bluetooth: ");
      Serial.println(data);

      int comma = data.indexOf(',');
      if (comma != -1) {
        int blinks = data.substring(0, comma).toInt();
        String gaze = data.substring(comma + 1);

        Serial.print("Blink Count: "); Serial.println(blinks);
        Serial.print("Gaze: "); Serial.println(gaze);

        // تنفيذ الأوامر
        if (gaze == "Looking Left") {
          turnLeft();
          delay(500);
          stopMoving();
        } else if (gaze == "Looking Right") {
          turnRight();
          delay(500);
          stopMoving();
        } else {
          if (blinks == 1) {
            moveForward();
            delay(700);
            stopMoving();
          } else if (blinks == 2) {
            stopMoving();
          } else if (blinks == 3) {
            moveBackward();
            delay(700);
            stopMoving();
          }
        }
      }
    }
  }
}
