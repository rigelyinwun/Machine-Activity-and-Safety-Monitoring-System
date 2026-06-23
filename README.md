# Machine-Activity-and-Safety-Monitoring-System

An integrated, closed-loop industrial safety system that combines **Computer Vision at the Edge** with an **IoT Hardware Node** to protect workers around hazardous machinery.

The system uses a **multi-modal decision pipeline (Vision + Sensors)** to detect real threats and trigger alarms only when necessary — reducing false positives while maintaining real-time responsiveness.

---

## System Architecture & Workflow

The system operates as a distributed Edge AI pipeline:

1. **Telemetry Generation (ESP32)**

   * Samples:

     * Vibration sensor (GPIO 34, Digital)
     * Sound sensor (GPIO 35, Analog)
   * Uses non-blocking timing for continuous monitoring

2. **MQTT Communication Layer**

   * Data serialized into JSON
   * Published every **1000 ms**
   * Topic: `industrial/machine/telemetry`
   * Protocol: MQTT (TCP Port 1883)

3. **Edge AI Vision Processing**

   * Camera resolution: `640 × 480`
   * Model: **YOLOv8n (Nano)**
   * Inference speed: ~**9.5 FPS**
   * Detection target: **Person (Class ID 0)**
   * Confidence threshold: `0.50`

4. **Closed-Loop Decision Control**

   * If:

     * Person detected **AND**
     * Machine state = `ABNORMAL`
   * Then:

     * Publish `{"alarm": true}` → `industrial/machine/control`
     * Trigger buzzer + warning lights

---

## System Pipeline

```
Camera → YOLOv8n Detection → MQTT Publish → ESP32 Processing
        → Sensor Fusion → Decision Logic → Alarm Trigger
```

---

## Hardware Component Setup

| Component                      | Pin / Specification                       | Description                  |
| ------------------------------ | ----------------------------------------- | ---------------------------- |
| **ESP32 Dev Kit**              | 3.3V Logic                                | Main controller              |
| **SW-18010P Vibration Sensor** | GPIO 34                                   | Digital vibration detection  |
| **KY-037 Sound Sensor**        | GPIO 35                                   | Analog sound level detection |
| **Active-Low Buzzer**          | GPIO 13                                   | Alarm output                 |
| **LED Indicators**             | 21 (Red), 22 (Yellow), 23 (Green)         | System status display        |
| **4×4 Matrix Keypad**          | Rows: 17, 5, 18, 19<br>Cols: 15, 2, 4, 16 | User input interface         |

---

## Getting Started

### 1. ESP32 Firmware Setup

Before compiling, you must create a secure configuration file:

#### Create `secret.h`

```cpp
#ifndef SECRET_H
#define SECRET_H

// Wi-Fi Credentials
const char* SECRET_SSID = "YOUR_WIFI_NAME";
const char* SECRET_PASS = "YOUR_WIFI_PASSWORD";

// MQTT Broker IP Address
const char* SECRET_MQTT = "192.168.X.X";

#endif
```

Place this file in the same directory as your `.ino` file.

#### Include in your code

```cpp
#include "secret.h"
```

#### Upload

* Open Arduino IDE
* Select ESP32 board
* Compile & upload

---

### 2. Edge AI Host Setup (Python)

#### Install dependencies

```bash
pip install opencv-python ultralytics paho-mqtt
```

#### Configure MQTT

Ensure this matches your ESP32:

```python
MQTT_BROKER = "192.168.X.X"
```

#### Run system

```bash
python edge_controller.py
```

---

### 3️. MQTT Broker

Run Mosquitto locally:

```bash
mosquitto
```

---

## Decision Logic (Core Intelligence)

The system uses **multi-modal validation**:

| Condition | Requirement                  |
| --------- | ---------------------------- |
| Vision    | Person detected (conf > 0.5) |
| Vibration | Machine movement detected    |
| Sound     | Above threshold              |

### Trigger Rule

```
IF (Person Detected)
AND (Vibration = TRUE)
AND (Sound > Threshold)
→ ACTIVATE ALARM
```

### Noise Rejection

```
IF (Sound ONLY)
AND (No Vibration)
→ IGNORE (environment noise)
```

---

## Smart Maintenance Mode

The system supports safe manual override using the keypad:

| Code   | Function                                            |
| ------ | --------------------------------------------------- |
| `123A` | Enable Maintenance Mode (Disable alarm, Yellow LED) |
| `789C` | Re-arm system (Return to normal operation)          |
| `*`    | Clear keypad input                                  |

### Maintenance Behavior

* Alarm is muted
* Vision triggers ignored
* System remains powered and monitored
* Visual indicator switches to **Yellow state**

---

## Performance Summary

| Metric     | Value                     |
| ---------- | ------------------------- |
| FPS        | ~9.5                      |
| Resolution | 640×480                   |
| Model      | YOLOv8n                   |
| Accuracy   | ~85–90% (normal lighting) |
| Latency    | Low (real-time capable)   |

---

## Design Highlights

* ✅ Edge AI + Embedded integration
* ✅ Real-time processing pipeline
* ✅ Multi-sensor fusion (Vision + Sound + Vibration)
* ✅ False-positive reduction logic
* ✅ Secure credential handling (`secret.h`)
* ✅ Human-in-the-loop override system

---

## Conclusion

This project demonstrates a **complete Edge AI safety system**, integrating real-time object detection with embedded hardware control. By combining multiple sensing modalities and intelligent decision logic, the system achieves **high reliability while minimizing false alarms**, making it suitable for real-world industrial safety applications.

---
