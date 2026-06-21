import cv2
import json
import time
from ultralytics import YOLO
import paho.mqtt.client as mqtt

# --- Configuration Settings ---
MQTT_BROKER = "192.168.0.3"  # Your laptop's IPv4 address
MQTT_PORT = 1883
TOPIC_SUB = "industrial/machine/telemetry"  # Listen to ESP32 data
TOPIC_PUB = "industrial/machine/control"    # Send commands to ESP32

# --- Rate Limiting Variables ---
PUBLISH_INTERVAL = 0.3  # Send data to ESP32 every 300 milliseconds (3 times a second)
last_publish_time = 0.0

# --- Shared Global Variables ---
latest_machine_state = "NORMAL"
latest_vibration = 0
latest_sound = 0

# MQTT Inbound Callback: Triggers instantly when the ESP32 publishes data
def on_message(client, userdata, msg):
    global latest_machine_state, latest_vibration, latest_sound
    try:
        # Decode binary network payload into a Python dictionary
        payload = json.loads(msg.payload.decode())
        latest_machine_state = payload.get("state", "NORMAL")
        latest_vibration = payload.get("vibration", 0)
        latest_sound = payload.get("sound", 0)
        
        print(f"[Telemetry RX] State: {latest_machine_state} | Vib: {latest_vibration} | Sound: {latest_sound}")
    except Exception as e:
        print(f"[Error] Failed to parse telemetry packet: {e}")

# Initialize MQTT Client Network Handshakes
print("[System] Connecting to local Mosquitto Broker...")
mqtt_client = mqtt.Client()
mqtt_client.on_message = on_message

try:
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
    mqtt_client.subscribe(TOPIC_SUB)
    # Start an autonomous background thread to process network keep-alives automatically
    mqtt_client.loop_start()
    print("✅ MQTT Networking Thread Started.")
except Exception as e:
    print(f"❌ [Network Error] Could not connect to MQTT Broker: {e}")
    print("Ensure Mosquitto broker is running locally via terminal/services.")
    exit(1)

# Initialize Edge AI Model Weights
print("[System] Loading YOLOv8 nano model weights...")
model = YOLO("yolov8n.pt") 

print("[System] Initializing webcam video stream...")
cap = cv2.VideoCapture(0) 

if not cap.isOpened():
    print("[Error] Could not open or access the webcam device.")
    mqtt_client.loop_stop()
    exit(1)

print("\n" + "="*60)
print(" INTEGRATED EDGE WORKSTATION LOOP RUNNING")
print("  -> Maintenance Authorization Passcode: 123A")
print("  -> Machine Restored / Fixed Passcode:  789C")
print("  -> Focus on the video pop-up and press 'q' to shut down.")
print("="*60 + "\n")

while True:
    ret, frame = cap.read()
    if not ret:
        print("[Error] Failed to grab frame from camera.")
        break

    # Run inference filtering exclusively for 'person' (Class ID 0)
    results = model(frame, verbose=False, conf=0.50, classes=[0]) # raise confidence level to 0.5
    person_detected = len(results[0].boxes) > 0
    
    # Plot standard bounding boxes and labels onto the frame
    annotated_frame = results[0].plot()

    # --- CRITICAL REAL-TIME SAFETY LOGIC DECISION TREE ---
    if person_detected and latest_machine_state == "ABNORMAL":
        alarm_payload = {"alarm": True}
        overlay_text = "🚨 CRITICAL HAZARD: HUMAN IN DANGER ZONE! 🚨"
        border_color = (0, 0, 255)  # Bright Red Alert
    else:
        alarm_payload = {"alarm": False}
        if latest_machine_state == "ABNORMAL":
            overlay_text = "⚠️ MACHINE ABNORMAL: Area Clear. Stand Back."
            border_color = (0, 165, 255)  # Orange Alert
        elif latest_machine_state == "WARNING":
            overlay_text = "⚙️ Machine Status: WARNING (Elevated Vibration)"
            border_color = (0, 255, 255)  # Yellow Alert
        elif latest_machine_state == "IDLE":
            overlay_text = "💤 Machine Status: IDLE (Awaiting Signal)"
            border_color = (255, 255, 0)  # Cyan
        else:
            overlay_text = "✅ System Status: ALL CLEAR & OPERATIONAL"
            border_color = (0, 255, 0)  # Safe Green

    # RATE-LIMITED NETWORK TRANSMISSION BLOCK
    current_time = time.time()
    if current_time - last_publish_time >= PUBLISH_INTERVAL:
        mqtt_client.publish(TOPIC_PUB, json.dumps(alarm_payload))
        last_publish_time = current_time

    # --- UI HUD RENDERING PIPELINE ---
    # Create a clean solid status banner block overlay at the top of the stream
    cv2.rectangle(annotated_frame, (0, 0), (frame.shape[1], 70), border_color, -1)
    
    # Render main warning contextual text (White text over colored banner)
    cv2.putText(annotated_frame, overlay_text, (15, 30), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
    
    # Print diagnostic telemetry data metrics directly inside the HUD screen
    metrics_text = f"Telemetry -> Vib: {latest_vibration} | Snd: {latest_sound} | Logic State: {latest_machine_state}"
    cv2.putText(annotated_frame, metrics_text, (15, 55), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)

    # Display the final integrated frame context window
    cv2.imshow("Industrial Edge Controller Hub (YOLOv8)", annotated_frame)

    # Gracefully break the execution loop when 'q' key is pressed
    if cv2.waitKey(1) & 0xFF == ord('q'):
        print("[System] Shutting down Edge Controller pipeline safely...")
        break

# Clean up hardware resources and network loop tasks
cap.release()
cv2.destroyAllWindows()
mqtt_client.loop_stop()
mqtt_client.disconnect()
print("Pipeline closed down successfully.")