import time
from flask import Flask, request, jsonify
from ultralytics import YOLO

app = Flask(__name__)
model = YOLO("yolov8n.pt")

@app.route("/predict", methods=["POST"])
def predict():

    file = request.files["file"]
    img_bytes = file.read()

    # ONLY inference timing (clean measurement)
    start = time.perf_counter()
    results = model(img_bytes)
    end = time.perf_counter()

    inference_time_ms = (end - start) * 1000

    return jsonify({
        "inference_time": inference_time_ms
    })

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)