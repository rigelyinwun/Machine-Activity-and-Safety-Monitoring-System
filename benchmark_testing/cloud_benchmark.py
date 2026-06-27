import time
import requests

URL = "http://<YOUR_EC2_PUBLIC_IP>:5000/predict"
IMAGE_PATH = "test.png"

iterations = 20

total_latencies = []
inference_times = []

print("🚀 CLOUD YOLO BENCHMARK START")

for i in range(iterations):
    start_time = time.perf_counter()
    with open(IMAGE_PATH, "rb") as f:
        response = requests.post(URL, files={"file": f})

    end_time = time.perf_counter()
    result = response.json()

    total_latency = (end_time - start_time) * 1000  
    inference_time = float(result.get("inference_time", 0)) * 1000  
    total_latencies.append(total_latency)
    inference_times.append(inference_time)

    print(f"Run {i+1}: Total = {total_latency:.2f} ms | Inference = {inference_time:.2f} ms")

avg_total = sum(total_latencies) / iterations
avg_inference = sum(inference_times) / iterations
avg_network = avg_total - avg_inference

print("\n===== CLOUD FINAL RESULT =====")
print(f"Average Total Latency: {avg_total:.2f} ms")
print(f"Average Inference Time: {avg_inference:.2f} ms")
print(f"Average Network Overhead: {avg_network:.2f} ms")