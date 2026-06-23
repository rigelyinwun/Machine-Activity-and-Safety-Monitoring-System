# Machine-Activity-and-Safety-Monitoring-System

An integrated, closed-loop industrial safety system that combines **Edge AI Computer Vision** with an **ESP32-based IoT sensor node** to monitor machine conditions and protect personnel in real-time. The system uses a **multi-modal decision model (vibration + sound + vision)** to accurately classify machine states and trigger safety responses only when necessary.

---

## System Architecture & Workflow

### End-to-End Pipeline

1. **Sensor Data Acquisition (ESP32)**

   * Vibration sensor (SW-18010P) measures machine activity via pulse counting.
   * Sound sensor (KY-037) captures acoustic intensity via ADC.
   * Data is processed every **1000 ms**.

2. **State Classification (Embedded Logic)**
   The ESP32 classifies machine condition into four states:

   | State                 | Condition                                   | LED       |
   | --------------------- | ------------------------------------------- | --------- |
   | **IDLE**              | Vibration = 0 OR Sound = 0                  | All OFF   |
   | **NORMAL (SAFE)**     | Vibration < threshold AND Sound < threshold | 🟢 Green  |
   | **WARNING**           | Vibration > threshold AND Sound < threshold | 🟡 Yellow |
   | **ABNORMAL (DANGER)** | Vibration > 0 AND Sound > threshold         | 🔴 Red    |

   ⚠️ Environmental noise is ignored when vibration = 0.

3. **MQTT Communication**

   * Data is serialized into JSON:

     ```json
     { "state": "ABNORMAL", "vibration": 15, "sound": 120 }
     ```
   * Published to:

     ```
     industrial/machine/telemetry
     ```
   * Broker: **MQTT (Port 1883)**

4. **Edge AI Vision Processing**

   * Camera stream processed using **YOLOv8n**
   * Resolution: **640×480**
   * FPS: ~**9–10 FPS**
   * Detects: **Person (Class 0)**

5. **Closed-Loop Safety Control**

   * If:

     * Machine = **ABNORMAL**
     * AND Person detected
   * Then:

     ```json
     { "alarm": true }
     ```
   * Sent to:

     ```
     industrial/machine/control
     ```
   * ESP32 triggers:

     * 🔴 Red LED
     * 🔊 Buzzer ON

---

## Hardware Setup

| Component                    | Pin              | Description                     |
| ---------------------------- | ---------------- | ------------------------------- |
| ESP32 Dev Kit                | —                | Main controller                 |
| Vibration Sensor (SW-18010P) | GPIO 34          | Pulse-based vibration detection |
| Sound Sensor (KY-037)        | GPIO 35          | Analog sound input              |
| Buzzer (Active-Low)          | GPIO 13          | Alarm output                    |
| Red LED                      | GPIO 21          | Danger indicator                |
| Yellow LED                   | GPIO 22          | Warning indicator               |
| Green LED                    | GPIO 23          | Safe indicator                  |
| 4x4 Keypad                   | Rows: 17,5,18,19; Columns: 15,2,4,16 | Maintenance input |

---

## System Logic (Core Behavior)

### Machine State Decision

```text
IF vibration == 0 → IDLE

ELSE IF vibration < threshold AND sound < threshold → NORMAL (SAFE)

ELSE IF vibration > threshold AND sound < threshold → WARNING

ELSE IF vibration > 0 AND sound > threshold → ABNORMAL
```

### Alarm Trigger Logic

```text
IF (Machine == ABNORMAL) AND (Person Detected)
    → Activate Buzzer + Red LED
ELSE
    → No alarm
```

---

## Smart Maintenance Mode (Keypad Control)

The system includes **human-in-the-loop override** via keypad:

| Function            | Password | Action                        |
| ------------------- | -------- | ----------------------------- |
| Maintenance Mode | `123C`   | Disable buzzer, Yellow LED ON |
| System Reset     | `789D`   | Re-enable monitoring          |
| Clear Input       | `*`      | Clear buffer                  |
| Confirm           | `#`      | Submit                        |

### Key Logic

* Maintenance mode sets:

  ```
  overrideActive = true
  ```
* Buzzer is **muted even if abnormal**
* After repair:

  * User enters reset password
  * System resumes monitoring

---

## MQTT Topics

| Direction    | Topic                          | Description  |
| ------------ | ------------------------------ | ------------ |
| ESP32 → Edge | `industrial/machine/telemetry` | Sensor data  |
| Edge → ESP32 | `industrial/machine/control`   | Alarm signal |

---

## Getting Started

### 1️. ESP32 Firmware Setup

Create a **`secrets.h`** file:

```cpp
#ifndef SECRETS_H
#define SECRETS_H

#define WIFI_SSID "YOUR_WIFI"
#define WIFI_PASSWORD "YOUR_PASSWORD"
#define MQTT_SERVER "192.168.X.X"
#define MQTT_PORT 1883

#endif
```

Include it in your `.ino`:

```cpp
#include "secrets.h"
```

---

### 2️. Python Edge AI Setup

Install dependencies:

```bash
pip install opencv-python ultralytics paho-mqtt
```

Run system:

```bash
python cameraDetect.py
```

---

## Performance Summary

| Feature         | Value        |
| --------------- | ------------ |
| Camera FPS      | ~9.5 FPS     |
| Detection Model | YOLOv8n      |
| Sensor Cycle    | 1000 ms      |
| Communication   | MQTT         |
| Latency         | ~1–2 seconds |

---

## Key Highlights

* ✅ Multi-sensor fusion (vibration + sound + vision)
* ✅ Intelligent noise filtering
* ✅ Real-time edge AI inference
* ✅ Human-safe maintenance override
* ✅ MQTT-based scalable architecture

---

## Conclusion

This project demonstrates a **robust industrial safety system** that balances automation with human control. By combining **embedded systems, edge AI, and IoT communication**, it delivers reliable hazard detection while avoiding false alarms—making it suitable for real-world deployment in smart factories.

---
