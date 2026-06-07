import cv2
import json
import paho.mqtt.client as mqtt
from ultralytics import YOLO

# --- Configuration ---
# Set this to your laptop's IP address if running a local broker
MQTT_BROKER = "175.143.167.143" 
TOPIC_SUB = "industrial/machine/telemetry" # Receive from ESP32
TOPIC_PUB = "industrial/machine/control"   # Send to ESP32

# Global variable to track the ESP32's reported state
machine_status = "IDLE"

# MQTT Callback: Runs when ESP32 sends sensor data
def on_message(client, userdata, msg):
    global machine_status
    data = json.loads(msg.payload.decode())
    machine_status = data.get("state", "IDLE")

# Initialize MQTT
client = mqtt.Client()
client.on_message = on_message
client.connect(MQTT_BROKER, 1883, 60)
client.subscribe(TOPIC_SUB)
client.loop_start() # Start networking in the background

# Load YOLOv8
model = YOLO("yolov8n.pt")
cap = cv2.VideoCapture(0) # Uses laptop webcam

while True:
    ret, frame = cap.read()
    if not ret: break

    # Run detection exclusively for 'person' (Class 0)
    results = model(frame, stream=True, classes=[0])
    person_detected = False

    for r in results:
        if len(r.boxes) > 0:
            person_detected = True
            # Optional: Draw boxes for your demonstration video
            for box in r.boxes:
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)

    # --- INTEGRATED LOGIC ---
    if person_detected and machine_status == "ABNORMAL":
        # Send "True" to trigger physical ESP32 buzzer
        client.publish(TOPIC_PUB, json.dumps({"alarm": True}))
        cv2.putText(frame, "⚠️ DANGER: HUMAN NEAR FAULT", (20, 50), 
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 3)
    else:
        # Send "False" to keep buzzer off
        client.publish(TOPIC_PUB, json.dumps({"alarm": False}))

    cv2.imshow("Industrial Safety Monitor", frame)
    if cv2.waitKey(1) & 0xFF == 27: break # 27 is the ASCII code for ESC

cap.release()
cv2.destroyAllWindows()
client.loop_stop()