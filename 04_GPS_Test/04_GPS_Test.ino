#include <TinyGPSPlus.h>

TinyGPSPlus gps;
HardwareSerial GPSserial(1);

void setup() {
  Serial.begin(115200);

  // RX and TX pins must match your wiring
  GPSserial.begin(9600, SERIAL_8N1, 16, 17);
}

void loop() {
  while (GPSserial.available()) {
    gps.encode(GPSserial.read());
  }

  if (gps.location.isValid()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);

    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("Waiting for GPS fix...");
  }

  delay(1000);
}