/*
  Elderly Fall Detection and Emergency Alert Band

  Hardware:
  - ESP32
  - MPU6050
  - GPS module such as NEO-6M
  - GSM module such as SIM800L or SIM900A
  - Buzzer
  - Cancel/SOS button

  Software:
  - Adafruit MPU6050
  - Adafruit Unified Sensor
  - Adafruit BusIO
  - TinyGPSPlus
  - TinyGSM
*/

#define TINY_GSM_MODEM_SIM800
// For SIM900A, use:
// #define TINY_GSM_MODEM_SIM900

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <TinyGPSPlus.h>

#include <TinyGsmClient.h>

// =====================================================
// USER SETTINGS
// =====================================================

// Caregiver phone number.
// Include the country code, for example +91XXXXXXXXXX.
const char CAREGIVER_NUMBER[] = "+91XXXXXXXXXX";

// Optional SIM PIN.
// Leave empty if your SIM does not use a PIN.
const char SIM_PIN[] = "";

// =====================================================
// PIN SETTINGS
// =====================================================

// MPU6050 I2C pins
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

// GPS UART pins
// ESP32 RX receives from GPS TX.
// ESP32 TX sends to GPS RX.
const int GPS_RX_PIN = 16;
const int GPS_TX_PIN = 17;

// GSM UART pins
// ESP32 RX receives from GSM TX.
// ESP32 TX sends to GSM RX.
const int GSM_RX_PIN = 27;
const int GSM_TX_PIN = 26;

// Buzzer and button
const int BUZZER_PIN = 25;
const int BUTTON_PIN = 33;

// =====================================================
// SERIAL SETTINGS
// =====================================================

const uint32_t USB_BAUD_RATE = 115200;
const uint32_t GPS_BAUD_RATE = 9600;
const uint32_t GSM_BAUD_RATE = 9600;

// =====================================================
// FALL DETECTION SETTINGS
// =====================================================

// Time during which the user can cancel the possible fall.
const unsigned long CANCEL_WINDOW_MS = 15000;

// Time between sensor readings.
const unsigned long SENSOR_INTERVAL_MS = 50;

// Time between debug messages.
const unsigned long DEBUG_INTERVAL_MS = 1000;

// A possible impact usually creates a temporary acceleration peak.
// These values must be calibrated using your actual band.
const float IMPACT_THRESHOLD_G = 2.50;

// Minimum change in orientation to support fall detection.
const float POSTURE_CHANGE_THRESHOLD_DEG = 45.0;

// If total acceleration is close to 1 g, the sensor is generally stationary.
const float STATIONARY_ACCEL_MIN_G = 0.75;
const float STATIONARY_ACCEL_MAX_G = 1.25;

// Time after the impact during which the system checks for inactivity.
const unsigned long POST_IMPACT_CHECK_MS = 2500;

// =====================================================
// OBJECTS
// =====================================================

Adafruit_MPU6050 mpu;
TinyGPSPlus gps;

HardwareSerial GPSSerial(1);
HardwareSerial GSMSerial(2);

TinyGsm modem(GSMSerial);

// =====================================================
// SYSTEM STATES
// =====================================================

enum SystemState {
  STATE_MONITORING,
  STATE_CONFIRMING,
  STATE_ALERT_SENT
};

SystemState systemState = STATE_MONITORING;

// =====================================================
// RUNTIME VARIABLES
// =====================================================

float uprightReferenceAngle = 0.0;
float currentAngle = 0.0;

float lastAccelerationG = 1.0;
float currentAccelerationG = 1.0;

unsigned long lastSensorReadTime = 0;
unsigned long lastDebugTime = 0;

unsigned long possibleFallTime = 0;
unsigned long impactTime = 0;

bool impactDetected = false;
bool postureChangeDetected = false;
bool inactivityDetected = false;
bool smsAlreadySent = false;

bool gpsHasFix = false;
double lastLatitude = 0.0;
double lastLongitude = 0.0;

bool modemReady = false;

// =====================================================
// FUNCTION DECLARATIONS
// =====================================================

void printSystemInfo();
void readGPSContinuously();
void updateGPSLocation();

bool initializeMPU6050();
bool initializeGSM();

void readMotion();
float calculateAccelerationG(float x, float y, float z);
float calculatePostureAngle(float x, float y, float z);

void updateFallDetection();
void startPossibleFall();
void handleConfirmationState();
void cancelPossibleFall();
void confirmFall();

void activateBuzzer();
void deactivateBuzzer();

bool isCancelButtonPressed();

bool sendEmergencySMS();
String createEmergencyMessage();

void printDebugInformation();
void resetToMonitoring();

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(USB_BAUD_RATE);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" Elderly Fall Detection Band");
  Serial.println(" ESP32 Software Starting");
  Serial.println("========================================");

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  GPSSerial.begin(
    GPS_BAUD_RATE,
    SERIAL_8N1,
    GPS_RX_PIN,
    GPS_TX_PIN
  );

  GSMSerial.begin(
    GSM_BAUD_RATE,
    SERIAL_8N1,
    GSM_RX_PIN,
    GSM_TX_PIN
  );

  Serial.println("Pins and serial ports initialized.");

  if (!initializeMPU6050()) {
    Serial.println("ERROR: MPU6050 initialization failed.");
    Serial.println("Check power, SDA, SCL, and ground connections.");

    while (true) {
      activateBuzzer();
      delay(200);
      deactivateBuzzer();
      delay(800);
    }
  }

  Serial.println("MPU6050 initialized successfully.");

  Serial.println("Starting GSM modem...");
  modemReady = initializeGSM();

  if (modemReady) {
    Serial.println("GSM modem initialized successfully.");
  } else {
    Serial.println("WARNING: GSM modem was not initialized.");
    Serial.println("The system will continue, but SMS may not work.");
  }

  Serial.println("Waiting for GPS location...");
  Serial.println("Place the GPS antenna near an open sky.");

  Serial.println();
  Serial.println("System is now monitoring for falls.");
  Serial.println("========================================");
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {
  unsigned long currentTime = millis();

  // GPS data must be read frequently.
  readGPSContinuously();
  updateGPSLocation();

  // Read and process motion at fixed intervals.
  if (currentTime - lastSensorReadTime >= SENSOR_INTERVAL_MS) {
    lastSensorReadTime = currentTime;

    readMotion();
    updateFallDetection();
  }

  // Handle the 15-second confirmation period.
  if (systemState == STATE_CONFIRMING) {
    handleConfirmationState();
  }

  // Print useful debug messages.
  if (currentTime - lastDebugTime >= DEBUG_INTERVAL_MS) {
    lastDebugTime = currentTime;
    printDebugInformation();
  }
}

// =====================================================
// MPU6050 INITIALIZATION
// =====================================================

bool initializeMPU6050() {
  if (!mpu.begin()) {
    return false;
  }

  // Accelerometer measurement range.
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

  // Gyroscope measurement range.
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  // Digital filter setting.
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  delay(100);

  return true;
}

// =====================================================
// GSM INITIALIZATION
// =====================================================

bool initializeGSM() {
  Serial.println("Checking GSM modem response...");

  // This may take several seconds.
  if (!modem.init()) {
    Serial.println("GSM modem did not respond to init().");
    return false;
  }

  Serial.print("Modem information: ");
  Serial.println(modem.getModemInfo());

  if (strlen(SIM_PIN) > 0) {
    modem.simUnlock(SIM_PIN);
  }

  Serial.println("Waiting for cellular network...");

  if (!modem.waitForNetwork(60000L)) {
    Serial.println("Network registration failed or timed out.");
    return false;
  }

  if (!modem.isNetworkConnected()) {
    Serial.println("GSM network is not connected.");
    return false;
  }

  Serial.println("GSM network connected.");
  return true;
}

// =====================================================
// GPS FUNCTIONS
// =====================================================

void readGPSContinuously() {
  while (GPSSerial.available() > 0) {
    char receivedCharacter = GPSSerial.read();
    gps.encode(receivedCharacter);
  }
}

void updateGPSLocation() {
  if (gps.location.isValid() && gps.location.age() < 5000) {
    gpsHasFix = true;

    lastLatitude = gps.location.lat();
    lastLongitude = gps.location.lng();
  }
}

// =====================================================
// MOTION FUNCTIONS
// =====================================================

void readMotion() {
  sensors_event_t accelerationEvent;
  sensors_event_t gyroscopeEvent;
  sensors_event_t temperatureEvent;

  mpu.getEvent(
    &accelerationEvent,
    &gyroscopeEvent,
    &temperatureEvent
  );

  float accelerationX = accelerationEvent.acceleration.x;
  float accelerationY = accelerationEvent.acceleration.y;
  float accelerationZ = accelerationEvent.acceleration.z;

  currentAccelerationG = calculateAccelerationG(
    accelerationX,
    accelerationY,
    accelerationZ
  );

  currentAngle = calculatePostureAngle(
    accelerationX,
    accelerationY,
    accelerationZ
  );

  // Track movement after the possible impact.
  if (impactDetected) {
    if (
      currentAccelerationG >= STATIONARY_ACCEL_MIN_G &&
      currentAccelerationG <= STATIONARY_ACCEL_MAX_G
    ) {
      inactivityDetected = true;
    }
  }

  lastAccelerationG = currentAccelerationG;
}

float calculateAccelerationG(float x, float y, float z) {
  const float STANDARD_GRAVITY = 9.80665;

  float accelerationMagnitude = sqrt(
    (x * x) +
    (y * y) +
    (z * z)
  );

  return accelerationMagnitude / STANDARD_GRAVITY;
}

float calculatePostureAngle(float x, float y, float z) {
  float horizontalMagnitude = sqrt(
    (x * x) +
    (y * y)
  );

  float angleRadians = atan2(horizontalMagnitude, z);
  float angleDegrees = angleRadians * 180.0 / PI;

  return angleDegrees;
}

// =====================================================
// FALL-DETECTION LOGIC
// =====================================================

void updateFallDetection() {
  if (systemState != STATE_MONITORING) {
    return;
  }

  // Detect a high acceleration spike.
  if (currentAccelerationG >= IMPACT_THRESHOLD_G) {
    impactDetected = true;
    impactTime = millis();

    Serial.println();
    Serial.println("High acceleration detected.");
    Serial.print("Acceleration: ");
    Serial.print(currentAccelerationG, 2);
    Serial.println(" g");

    return;
  }

  // After the impact, wait briefly for a posture change.
  if (impactDetected) {
    unsigned long timeSinceImpact = millis() - impactTime;

    if (fabs(currentAngle - uprightReferenceAngle) >=
        POSTURE_CHANGE_THRESHOLD_DEG) {
      postureChangeDetected = true;
    }

    // Evaluate the possible fall after the post-impact period.
    if (timeSinceImpact >= POST_IMPACT_CHECK_MS) {
      if (postureChangeDetected || inactivityDetected) {
        startPossibleFall();
      } else {
        Serial.println("Movement did not satisfy fall conditions.");
        impactDetected = false;
        postureChangeDetected = false;
        inactivityDetected = false;
      }
    }
  }
}

void startPossibleFall() {
  systemState = STATE_CONFIRMING;
  possibleFallTime = millis();
  smsAlreadySent = false;

  activateBuzzer();

  Serial.println();
  Serial.println("========================================");
  Serial.println("POSSIBLE FALL DETECTED");
  Serial.println("Press the cancel button within 15 seconds.");
  Serial.println("========================================");
}

void handleConfirmationState() {
  // The user can cancel a false alarm.
  if (isCancelButtonPressed()) {
    cancelPossibleFall();
    return;
  }

  unsigned long confirmationTime =
    millis() - possibleFallTime;

  if (confirmationTime >= CANCEL_WINDOW_MS) {
    confirmFall();
  }
}

void cancelPossibleFall() {
  Serial.println();
  Serial.println("Alert cancelled by the user.");

  deactivateBuzzer();
  resetToMonitoring();
}

void confirmFall() {
  if (smsAlreadySent) {
    return;
  }

  Serial.println();
  Serial.println("No cancellation received.");
  Serial.println("Fall treated as an emergency.");

  smsAlreadySent = true;
  systemState = STATE_ALERT_SENT;

  // Keep buzzer active to alert people nearby.
  activateBuzzer();

  bool smsResult = sendEmergencySMS();

  if (smsResult) {
    Serial.println("Emergency SMS sent successfully.");
  } else {
    Serial.println("Emergency SMS could not be sent.");
  }

  Serial.println("Local buzzer remains active.");
}

// =====================================================
// BUTTON AND BUZZER
// =====================================================

bool isCancelButtonPressed() {
  static unsigned long lastButtonTime = 0;

  if (digitalRead(BUTTON_PIN) == LOW) {
    if (millis() - lastButtonTime > 500) {
      lastButtonTime = millis();
      return true;
    }
  }

  return false;
}

void activateBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
}

void deactivateBuzzer() {
  digitalWrite(BUZZER_PIN, LOW);
}

// =====================================================
// SMS FUNCTIONS
// =====================================================

String createEmergencyMessage() {
  String message;

  message.reserve(220);

  message += "Fall detected! Please check immediately.";

  if (gpsHasFix) {
    message += " Location: https://maps.google.com/?q=";
    message += String(lastLatitude, 6);
    message += ",";
    message += String(lastLongitude, 6);
  } else {
    message += " GPS location is currently unavailable.";
  }

  return message;
}

bool sendEmergencySMS() {
  if (!modemReady) {
    Serial.println("GSM modem is not ready.");
    return false;
  }

  String message = createEmergencyMessage();

  Serial.println("Preparing emergency SMS:");
  Serial.println(message);

  bool result = modem.sendSMS(
    CAREGIVER_NUMBER,
    message
  );

  return result;
}

// =====================================================
// DEBUG INFORMATION
// =====================================================

void printDebugInformation() {
  Serial.println();
  Serial.println("--------------- STATUS ---------------");

  Serial.print("System state: ");

  if (systemState == STATE_MONITORING) {
    Serial.println("MONITORING");
  } else if (systemState == STATE_CONFIRMING) {
    Serial.println("CONFIRMING POSSIBLE FALL");
  } else if (systemState == STATE_ALERT_SENT) {
    Serial.println("ALERT SENT");
  }

  Serial.print("Acceleration: ");
  Serial.print(currentAccelerationG, 2);
  Serial.println(" g");

  Serial.print("Posture angle: ");
  Serial.print(currentAngle, 2);
  Serial.println(" degrees");

  Serial.print("GPS fix: ");
  Serial.println(gpsHasFix ? "YES" : "NO");

  if (gpsHasFix) {
    Serial.print("Latitude: ");
    Serial.println(lastLatitude, 6);

    Serial.print("Longitude: ");
    Serial.println(lastLongitude, 6);
  }

  Serial.print("GSM ready: ");
  Serial.println(modemReady ? "YES" : "NO");

  Serial.println("--------------------------------------");
}

// =====================================================
// RESET SYSTEM
// =====================================================

void resetToMonitoring() {
  systemState = STATE_MONITORING;

  impactDetected = false;
  postureChangeDetected = false;
  inactivityDetected = false;

  possibleFallTime = 0;
  impactTime = 0;

  Serial.println("System returned to monitoring mode.");
}