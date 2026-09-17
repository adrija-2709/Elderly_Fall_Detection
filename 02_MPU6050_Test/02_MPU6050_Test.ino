#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);

  if (!mpu.begin()) {
    Serial.println("MPU6050 not detected");
    while (true);
  }

  Serial.println("MPU6050 detected");
}

void loop() {
  sensors_event_t acceleration;
  sensors_event_t rotation;
  sensors_event_t temperature;

  mpu.getEvent(&acceleration, &rotation, &temperature);

  Serial.print("X acceleration: ");
  Serial.println(acceleration.acceleration.x);

  Serial.print("Y acceleration: ");
  Serial.println(acceleration.acceleration.y);

  Serial.print("Z acceleration: ");
  Serial.println(acceleration.acceleration.z);

  delay(500);
}