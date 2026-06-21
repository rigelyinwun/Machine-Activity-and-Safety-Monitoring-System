from ultralytics import YOLO
import time

model = YOLO("yolov8n.pt")
img = "test.png"

total_times = []
inference_times = []

print("🚀 EDGE YOLO BENCHMARK START")

# Warm-up runs (important for stable GPU/CPU performance)
for _ in range(5):
    model(img)

for i in range(20):

    start_time = time.perf_counter()
    results = model(img)
    end_time = time.perf_counter()

    total_latency = (end_time - start_time) * 1000
    inference_time = results[0].speed["inference"]

    total_times.append(total_latency)
    inference_times.append(inference_time)

    print(f"Run {i+1}: Total={total_latency:.2f} ms | Inference={inference_time:.2f} ms")

avg_total = sum(total_times) / len(total_times)
avg_inference = sum(inference_times) / len(inference_times)
avg_network = avg_total - avg_inference

print("\n===== EDGE FINAL RESULT =====")
print(f"Average Total Latency: {avg_total:.2f} ms")
print(f"Average Inference Time: {avg_inference:.2f} ms")
print(f"Average Network Overhead: {avg_network:.2f} ms")