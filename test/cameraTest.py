import cv2
from ultralytics import YOLO

def test_camera_detection():
    print("[System] Loading YOLOv8 nano model weights...")
    # Automatically downloads 'yolov8n.pt' on the first run if not present
    model = YOLO("yolov8n.pt") 
    
    print("[System] Initializing webcam video stream...")
    # '0' is usually the default built-in laptop webcam or first connected USB camera
    cap = cv2.VideoCapture(0) 

    if not cap.isOpened():
        print("[Error] Could not open or access the webcam device.")
        return

    print("\n" + "="*50)
    print(" YOLOv8 CAMERA TEST RUNNING")
    print(" -> Look at the pop-up window.")
    print(" -> Stand in front of the camera to verify human detection.")
    print(" -> Press 'q' while focusing on the video window to quit.")
    print("="*50 + "\n")

    while True:
        ret, frame = cap.read()
        if not ret:
            print("[Error] Failed to grab frame from camera.")
            break

        # Run inference filtering exclusively for 'person' (Class ID 0)
        results = model(frame, verbose=False, classes=[0])
        
        # Count how many humans are currently detected in the frame context
        person_count = len(results[0].boxes)
        
        # Plot standard bounding boxes and labels onto the current video frame
        annotated_frame = results[0].plot()

        # Add a custom visual overlay showing real-time detection status
        if person_count > 0:
            status_text = f"STATUS: Person Detected ({person_count})"
            text_color = (0, 0, 255) # Red text alert
        else:
            status_text = "STATUS: Scanning for People..."
            text_color = (0, 255, 0) # Green text safe

        cv2.putText(annotated_frame, status_text, (20, 40), 
                    cv2.FONT_HERSHEY_SIMPLEX, 1, text_color, 2)

        # Display the live rendering pipeline output window
        cv2.imshow("YOLOv8 Human Detection Verification Test", annotated_frame)

        # Gracefully break the infinite execution loop when the 'q' key is pressed
        if cv2.waitKey(1) & 0xFF == ord('q'):
            print("[System] Exiting verification test script.")
            break

    # Release hardware allocation and cleanly destroy OpenCV rendering contexts
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    test_camera_detection()