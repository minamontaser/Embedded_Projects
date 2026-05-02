## 🚀 Features

* 🌡️ Multi-sensor environmental monitoring

  * DHT11 (temperature & humidity)
  * LM35 & TMP36 analog temperature sensors
  * LDR light sensor

* 📡 Security system

  * PIR motion detection
  * Ultrasonic distance-based intrusion detection (HC-SR04)

* 🎮 Joystick-controlled UI

  * Menu navigation system
  * Interactive selection interface

* 🔐 PIN-based authentication

  * Secure shutdown protection
  * Logout PIN in Guardian mode
  * Lockout after failed attempts

* 🚨 Alarm system

  * Buzzer alerts
  * LED indicators (Red, Green, Yellow)
  * Blinking alarm state

* 📟 LCD interface (16x2)

  * Smooth UI updates
  * Menu + live sensor display

---

## 🧰 Hardware Requirements

* Arduino Mega 2560
* DHT11 sensor
* LM35 temperature sensor
* TMP36 temperature sensor
* LDR (Light Dependent Resistor)
* HC-SR04 ultrasonic sensor
* PIR motion sensor
* 16x2 LCD display (parallel interface)
* Joystick module
* Active buzzer
* 3x LEDs (Red, Green, Yellow)
* Resistors & jumper wires

---

## 🔌 Pin Configuration

| Component       | Pin                    |
| --------------- | ---------------------- |
| DHT11           | 7                      |
| Ultrasonic TRIG | 9                      |
| Ultrasonic ECHO | 4                      |
| PIR Sensor      | 3                      |
| Red LED         | 8                      |
| Green LED       | 11                     |
| Yellow LED      | 12                     |
| Buzzer          | 5                      |
| Joystick X      | A12                    |
| Joystick Y      | A13                    |
| Joystick Button | 28                     |
| LM35            | A2                     |
| TMP36           | A3                     |
| LDR             | A4                     |
| LCD             | 48, 46, 41, 40, 39, 38 |

---

## ⚙️ System Modes

### 🟢 Normal Mode

* Displays environmental data:

  * Temperature (fused from multiple sensors using median filtering)
  * Humidity
  * Light intensity
* Green LED ON
* No alarms active

---

### 🔴 Guardian Mode

* Security monitoring active:

  * PIR motion detection
  * Ultrasonic distance threshold (< 30 cm)
* Alarm triggers:

  * Buzzer ON
  * Red + Yellow LED blinking
* LCD shows real-time distance

---

### ⚫ Shutdown Mode

* Secure system shutdown
* Requires **4-digit PIN (default: 1234)**
* Prevents unauthorized shutdown
* After 3 wrong logout attempts → system enters Guardian alarm state

---

## 🎮 Controls (Joystick)

* ↑ / ↓ → Navigate menu or change values
* ← / → → Switch items / move cursor
* Button → Confirm selection / enter PIN

---

## 🔐 Security Logic

* PIN required for:

  * System shutdown
  * Secure logout from Guardian mode
* Anti-bruteforce mechanism:

  * 3 wrong attempts trigger alarm mode
* System locks into Guardian mode on repeated failure

---

## 📊 Sensor Processing

* **Temperature Fusion**

  * Uses median filtering between:

    * LM35
    * TMP36
    * DHT11
  * Improves accuracy and stability

* **Distance Filtering**

  * Moving average over 5 samples for ultrasonic sensor

---

## 🧠 System Architecture

* Menu-driven finite state machine:

  * `NORMAL`
  * `GUARDIAN`
  * `SHUTDOWN`
* PIN entry subsystem
* Sensor abstraction layer
* Real-time LCD rendering engine (optimized updates)

---

## 📦 How It Works

1. System boots and initializes all sensors
2. User navigates menu using joystick
3. Select mode:

   * Normal → monitoring dashboard
   * Guardian → security system
   * Shutdown → PIN-protected power-off
4. Sensors continuously update LCD + triggers
5. Alarm system activates on intrusion or failed security attempts

---
