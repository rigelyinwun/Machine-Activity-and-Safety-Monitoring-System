from ultralytics import YOLO
import time

model = YOLO("yolov8n.pt")
img = "test.png"

t = []
inf = []

print("🚀 EDGE TEST START")

# Warm-up
for _ in range(5):
    model(img, verbose=False)

for k in range(20):

    start = time.perf_counter()

    r = model(img, verbose=False)

    end = time.perf_counter()

    total_latency = (end - start) * 1000
    inference_time = r[0].speed["inference"]

    t.append(total_latency)
    inf.append(inference_time)

    print(
        f"Run {k+1}: "
        f"Total={total_latency:.2f}ms | "
        f"Inference={inference_time:.2f}ms"
    )

avg_total = sum(t) / len(t)
avg_inf = sum(inf) / len(inf)
avg_overhead = avg_total - avg_inf

print("\n===== EDGE FINAL RESULT =====")
print(f"Average Total Latency: {avg_total:.2f} ms")
print(f"Average Inference Time: {avg_inf:.2f} ms")
print(f"Average Network Overhead: {avg_overhead:.2f} ms")