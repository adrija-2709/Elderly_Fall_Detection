# Elderly_Fall_Detection

# Elderly Fall Detection & Emergency Alert Band

A wearable safety system that detects a possible fall, gives the wearer a short time to cancel a false alarm, and sends an emergency SMS containing the user’s GPS location to a registered caregiver.

This README explains the project idea, hardware, software, setup, code structure, operating mechanism, testing procedure, and troubleshooting instructions.

***

## 1. Project Overview

### Problem

An elderly person may fall while alone and be unable to reach a phone or manually call for help. Delayed assistance can increase the severity of the injury.

### Proposed solution

The band continuously monitors movement using an **MPU6050 motion sensor**. The ESP32 analyzes the motion data. If the data resembles a fall, the device activates a buzzer and starts a cancellation countdown.

If the wearer is safe, they can press the cancel button. If the button is not pressed within the configured time, the system:

1. Treats the event as an emergency.
2. Reads the latest available GPS coordinates.
3. Creates a Google Maps location link.
4. Sends an SMS through the GSM module.
5. Keeps the local buzzer active so nearby people may notice.

### Important note

This is an educational prototype and is not a certified medical or life-safety device. The fall-detection thresholds require testing and calibration before the system can be considered reliable.

***

## 2. Main Features

- Automatic motion monitoring.
- Possible-fall detection using acceleration and posture change.
- 10–15 second false-alarm cancellation period.
- Audible buzzer warning.
- Manual cancellation through a push button.
- GPS-based location acquisition.
- Emergency SMS through GSM.
- Google Maps location link.
- Separate testing of each hardware module.
- Expandable architecture for future mobile apps, health monitoring, and cloud services.

***

## 3. System Mechanism

The complete mechanism is:

```text
MPU6050
   ↓
ESP32 reads acceleration and posture
   ↓
Possible impact detected
   ↓
Posture change and inactivity checked
   ↓
Buzzer starts
   ↓
15-second cancellation period
   ↓
Cancel button pressed?
   ├── Yes → Stop buzzer and return to monitoring
   └── No → Confirm emergency
                 ↓
            Read GPS location
                 ↓
            Send GSM SMS
                 ↓
            Keep buzzer active
```

### Normal monitoring

The ESP32 continuously reads the MPU6050. During normal operation, the system remains in the monitoring state.

### Impact detection

A possible impact is identified when the total acceleration exceeds a configured threshold.

The total acceleration magnitude is calculated approximately as:

\[
a = \sqrt{a_x^2 + a_y^2 + a_z^2}
\]

The result is converted to units of \(g\), where \(1g\) represents normal gravitational acceleration.

### Posture detection

The program calculates an approximate posture angle using the accelerometer axes. A large change from the reference position suggests that the band may have changed from an upright to a horizontal orientation.

### Inactivity detection

After an impact, the program checks whether the device becomes relatively stationary. This helps distinguish a possible fall from ordinary rapid movement.

### Confirmation stage

When impact, posture change, or post-impact inactivity satisfy the programmed conditions:

- The system enters the confirmation state.
- The buzzer activates.
- A timer starts.
- The user can press the button to cancel the alert.

The program uses `millis()` rather than a long `delay()` so that GPS data and button presses can continue to be processed during the countdown.

### Emergency stage

If the cancellation period expires:

- The system confirms the event as an emergency.
- It reads the most recent valid GPS coordinates.
- It creates a message.
- It sends the message using the GSM module.
- It continues operating the buzzer.

If GPS does not have a valid fix, the SMS reports that the location is unavailable.

***

## 4. Hardware Components

| Component | Function |
|---|---|
| ESP32 development board | Main controller; processes data and controls all modules. |
| MPU6050 | Measures three-axis acceleration and three-axis angular motion. |
| GPS module | Provides latitude and longitude. |
| GSM module | Sends emergency SMS. |
| Buzzer | Provides a local audible alert. |
| Push button | Cancels a false alert. |
| Battery or power supply | Powers the system. |
| Jumper wires and breadboard/PCB | Used for prototyping and connections. |

The repository code assumes a common ESP32 development board, MPU6050, GPS module such as NEO-6M, and SIM800-family GSM module. The exact pins may be changed in the configuration section.

***

## 5. Example Pin Configuration

The default code uses the following pins.

### MPU6050

| MPU6050 pin | ESP32 pin |
|---|---|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| GND | GND |
| VCC | Suitable module supply |

### GPS

| GPS pin | ESP32 pin |
|---|---|
| TX | GPIO 16 |
| RX | GPIO 17 |
| GND | GND |
| VCC | Suitable module supply |

### GSM

| GSM pin | ESP32 pin |
|---|---|
| TX | GPIO 27 |
| RX | GPIO 26 |
| GND | Common GND |
| VCC | Separate suitable GSM supply |

### Buzzer and button

| Component | ESP32 pin |
|---|---|
| Buzzer positive | GPIO 25 |
| Buzzer negative | GND |
| Button | GPIO 33 and GND |

Serial communication must be crossed:

```text
ESP32 RX ← Module TX
ESP32 TX → Module RX
```

The exact wiring must match the constants in the code.

***

## 6. Power Requirements

Power must be handled carefully.

### ESP32

The ESP32 can usually be powered through its USB connector during development.

### MPU6050

Use the voltage recommended by the specific breakout board. Some boards include a regulator and level shifting; others may require 3.3 V.

### GPS

Use the voltage specified by the GPS breakout board.

### GSM

The GSM module should use a separate stable supply capable of handling current bursts during network transmission. Do not power a SIM800L directly from the ESP32 3.3 V pin.

All modules must share a common ground:

```text
ESP32 GND
MPU6050 GND
GPS GND
GSM GND
Buzzer GND
```

***

## 7. Repository Structure

A recommended GitHub repository structure is:

```text
elderly-fall-detection-band/
│
├── README.md
│
├── src/
│   └── elderly_fall_detection_band/
│       └── elderly_fall_detection_band.ino
│
├── tests/
│   ├── esp32_blink_test/
│   │   └── esp32_blink_test.ino
│   ├── mpu6050_test/
│   │   └── mpu6050_test.ino
│   ├── gps_test/
│   │   └── gps_test.ino
│   ├── gsm_test/
│   │   └── gsm_test.ino
│   └── buzzer_button_test/
│       └── buzzer_button_test.ino
│
├── docs/
│   ├── wiring.md
│   ├── testing.md
│   └── calibration.md
│
└── .gitignore
```

The main final program should be located at:

```text
src/elderly_fall_detection_band/
elderly_fall_detection_band.ino
```

Each Arduino sketch should be inside a folder with the same name as the `.ino` file. This avoids Arduino IDE sketch-folder errors.

***

## 8. Software Requirements

The team member needs:

- Arduino IDE 2.x.
- ESP32 board package by Espressif Systems.
- Adafruit MPU6050 library.
- Adafruit Unified Sensor library.
- Adafruit BusIO library.
- TinyGPSPlus library.
- TinyGSM library.
- USB-to-serial driver if the ESP32 board requires one.
- A USB data cable.
- A valid SIM card for GSM testing.

The ESP32 Arduino platform is installed through Arduino IDE’s Boards Manager. Libraries can be installed through Arduino IDE’s Library Manager. [docs.arduino](https://docs.arduino.cc/software/ide-v2)

TinyGPSPlus parses GPS NMEA sentences into usable location values such as latitude and longitude. [docs.arduino](https://docs.arduino.cc/libraries/tinygpsplus/)

TinyGSM provides support for common GSM modules, including SIM800 and SIM900 families. [docs.arduino](https://docs.arduino.cc/libraries/tinygsm/)

***

## 9. Arduino IDE Setup

### Step 1: Install Arduino IDE

Install Arduino IDE 2.x on Windows, macOS, or Linux.

Open Arduino IDE after installation.

### Step 2: Add ESP32 board URL

Open:

```text
File → Preferences
```

Add the following URL under **Additional Boards Manager URLs**:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Click **OK**.

### Step 3: Install ESP32 board package

Open:

```text
Tools → Board → Boards Manager
```

Search for:

```text
esp32
```

Install:

```text
esp32 by Espressif Systems
```

### Step 4: Select the board

Open:

```text
Tools → Board → ESP32 Arduino
```

For a common ESP32-WROOM development board, select:

```text
ESP32 Dev Module
```

If the physical board has a specific model, select that model instead.

### Step 5: Select the port

Connect the ESP32 using a data USB cable.

Open:

```text
Tools → Port
```

Select the port belonging to the ESP32.

If the port is missing:

- Try another cable.
- Try another USB port.
- Install the correct USB-to-serial driver.
- Restart Arduino IDE.
- Reconnect the ESP32.

### Step 6: Install libraries

Open:

```text
Sketch → Include Library → Manage Libraries
```

Install:

```text
Adafruit MPU6050
Adafruit Unified Sensor
Adafruit BusIO
TinyGPSPlus
TinyGSM
```

The Serial Monitor is used for startup messages, sensor values, GPS status, GSM responses, and debugging. [docs.arduino](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-serial-monitor/)

***

## 10. Configuration Before Uploading

Open the final `.ino` file and edit the settings near the top.

### Caregiver number

Replace:

```cpp
const char CAREGIVER_NUMBER[] = "+91XXXXXXXXXX";
```

with the actual number, including country code.

Example format:

```cpp
const char CAREGIVER_NUMBER[] = "+919876543210";
```

Do not commit personal phone numbers to a public GitHub repository. Use a private repository or a separate local configuration file.

### GSM modem

For SIM800:

```cpp
#define TINY_GSM_MODEM_SIM800
```

For SIM900:

```cpp
#define TINY_GSM_MODEM_SIM900
```

Only define one modem type.

### Pins

Verify these values against the actual wiring:

```cpp
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

const int GPS_RX_PIN = 16;
const int GPS_TX_PIN = 17;

const int GSM_RX_PIN = 27;
const int GSM_TX_PIN = 26;

const int BUZZER_PIN = 25;
const int BUTTON_PIN = 33;
```

### GSM baud rate

The default is:

```cpp
const uint32_t GSM_BAUD_RATE = 9600;
```

Change it if the GSM module uses another baud rate.

### Fall thresholds

The initial values are:

```cpp
const float IMPACT_THRESHOLD_G = 2.50;
const float POSTURE_CHANGE_THRESHOLD_DEG = 45.0;
```

These are starting values only. They require calibration.

***

## 11. Upload Procedure

1. Open the final `.ino` file.
2. Select the ESP32 board.
3. Select the correct port.
4. Click **Verify**.
5. Wait for compilation to finish.
6. Click **Upload**.
7. If upload fails, hold the ESP32 BOOT button while uploading.
8. Release the BOOT button when the upload begins.
9. Open:

```text
Tools → Serial Monitor
```

10. Set the baud rate to:

```text
115200
```

The code should display messages such as:

```text
Elderly Fall Detection Band
ESP32 Software Starting
MPU6050 initialized successfully
GSM modem initialized successfully
System is now monitoring for falls
```

***

## 12. Recommended Testing Order

Do not begin with the complete final system. Test each part separately.

### Test 1: ESP32

Upload a blinking LED sketch.

Purpose:

- Confirm the board is detected.
- Confirm the port works.
- Confirm code uploads correctly.

### Test 2: MPU6050

Upload the MPU6050 test sketch.

Confirm:

- The sensor is detected.
- Acceleration values are displayed.
- Values change when the sensor moves.

### Test 3: Buzzer and button

Upload the buzzer/button test.

Confirm:

- The buzzer can be activated.
- The button changes state.
- Pressing the button can stop the buzzer.

### Test 4: GPS

Upload the GPS test.

Confirm:

- Serial GPS data is received.
- A valid latitude and longitude are obtained.
- The GPS antenna has a clear view of the sky.

### Test 5: GSM

Upload the GSM test.

Confirm:

- The modem responds.
- The SIM card is recognized.
- The modem is registered on the network.
- A normal test SMS can be sent.

### Test 6: Complete system

Upload the final program.

Confirm:

- Normal monitoring works.
- The buzzer starts after a simulated possible fall.
- The cancel button stops a false alarm.
- SMS is sent if the button is not pressed.
- The location link opens correctly.
- The buzzer remains active after the emergency SMS.

***

## 13. Serial Monitor Messages

The Serial Monitor helps identify which stage is failing.

Expected examples:

```text
MPU6050 initialized successfully.
Starting GSM modem...
GSM network connected.
Waiting for GPS location...
System is now monitoring for falls.
```

During motion:

```text
Acceleration: 1.02 g
Posture angle: 14.30 degrees
GPS fix: YES
GSM ready: YES
```

During a possible fall:

```text
High acceleration detected.
POSSIBLE FALL DETECTED
Press the cancel button within 15 seconds.
```

If cancelled:

```text
Alert cancelled by the user.
System returned to monitoring mode.
```

If confirmed:

```text
No cancellation received.
Fall treated as an emergency.
Preparing emergency SMS:
Fall detected! Please check immediately.
Location: https://maps.google.com/?q=...
Emergency SMS sent successfully.
```

***

## 14. Troubleshooting

### ESP32 does not appear

Check:

- USB cable supports data.
- USB-to-serial driver is installed.
- Correct port is selected.
- Board is receiving power.
- Arduino IDE has been restarted.

### Upload fails

Try:

- Pressing and holding BOOT while uploading.
- Selecting `ESP32 Dev Module`.
- Closing the Serial Monitor before uploading.
- Selecting the correct port.
- Using a shorter USB cable.

### MPU6050 is not detected

Check:

- SDA and SCL.
- VCC and GND.
- Common ground.
- I2C address.
- Whether the sensor is damaged.

### GPS has no fix

Check:

- GPS antenna connection.
- Outdoor or open-sky location.
- TX/RX crossing.
- Correct baud rate.
- GPS power supply.
- Whether the program continuously calls `gps.encode()`.

GPS may not obtain a position indoors.

### GSM does not respond

Check:

- GSM power supply.
- SIM card installation.
- SIM card activation.
- Network signal.
- Antenna.
- Module baud rate.
- TX/RX crossing.
- Common ground.

### SMS does not arrive

Check:

- The caregiver number includes the country code.
- The SIM has SMS service.
- The GSM module is network-registered.
- Signal strength is sufficient.
- The number is not blocked.
- The modem is using the correct configuration.

### Too many false detections

Adjust:

```cpp
IMPACT_THRESHOLD_G
POSTURE_CHANGE_THRESHOLD_DEG
POST_IMPACT_CHECK_MS
```

Also require more conditions before confirming a fall, such as:

- Impact.
- Significant posture change.
- Post-impact inactivity.
- No button response.

### Fall is not detected

Possible causes:

- Threshold is too high.
- Sensor is loose.
- Sensor orientation differs from the expected orientation.
- The impact is too soft.
- The posture-change threshold is too large.

Record sensor values during safe, controlled movement and adjust the thresholds.

***




Each user should enter their test number locally.

For a better implementation, create a separate local configuration file that is excluded through `.gitignore`.

***

## 16. Limitations

- Motion-only detection can produce false positives and false negatives.
- GPS may not work reliably indoors.
- GPS location is not always instantly available.
- GSM depends on network coverage.
- GSM modules require careful power design.
- The band’s orientation affects posture-angle calculations.
- The system is not a certified medical alert device.
- A wrist-mounted sensor may detect hand movements that resemble falls.

***

## 17. Future Improvements

Possible improvements include:

- Separate cancel and SOS buttons.
- Vibration motor for silent alerts.
- Battery-level monitoring.
- Low-battery SMS.
- Heart-rate and SpO₂ sensing.
- Mobile application.
- Cloud-based caregiver dashboard.
- Fall-history storage.
- Better orientation estimation using sensor fusion.
- Machine-learning-based fall classification.
- Waterproof enclosure.
- Power-saving sleep modes.
- Cellular modules compatible with currently available networks.

***

## 18. Workflow

These steps should be followed:

1. Clone the GitHub repository.
2. Install Arduino IDE 2.x.
3. Install the ESP32 board package.
4. Install all required libraries.
5. Open the ESP32 blink test.
6. Select the correct board and port.
7. Upload the blink test.
8. Open the MPU6050 test.
9. Confirm motion readings.
10. Test GPS independently.
11. Test GSM independently.
12. Check the wiring against the repository documentation.
13. Edit local phone-number and modem settings.
14. Upload the final program.
15. Run the test plan.
16. Record all threshold changes and observations.
17. Commit only tested code and documentation.

***

## 19. Purpose

This project uses the ESP32 as the main controller. The MPU6050 continuously measures movement and orientation. When the ESP32 detects a sudden acceleration spike followed by a significant posture change or inactivity, it identifies a possible fall. The buzzer starts and the user receives a short time to press the cancel button. If the user does not respond, the ESP32 obtains the latest GPS coordinates and instructs the GSM module to send an emergency SMS to the caregiver. The SMS contains a Google Maps link whenever a valid GPS position is available.

The project software is developed in Arduino IDE using the ESP32 board package, the Adafruit MPU6050 library, TinyGPSPlus, and TinyGSM. Each module must be tested separately before the complete system is assembled in software.
