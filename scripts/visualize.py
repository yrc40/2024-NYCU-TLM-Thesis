import re
import matplotlib.pyplot as plt
from datetime import datetime

file_path = "result.txt"
with open(file_path, "r") as file:
    raw_data = file.read()

bus_event_pattern = re.compile(
    r"Time:\s(?P<time>[\d:]+)\n(?:New Event:\s)?Bus\s(?P<bus_id>\d+)\s(?P<event>arrive at|depart from|arrive at light|depart from light)\s(?:stop|light)?\s?(?P<stop_id>\d+)?\n(?:mileage\s=\s(?P<mileage>\d+))?"
)

bus_data = {}

for match in bus_event_pattern.finditer(raw_data):
    print(f"Matched: {match.groupdict()}") 
    
    time_str = match.group("time")
    bus_id = int(match.group("bus_id"))
    event = match.group("event")
    stop_id = match.group("stop_id")
    mileage = match.group("mileage")

    time_obj = datetime.strptime(time_str, "%H:%M:%S")
    time_in_seconds = time_obj.hour * 3600 + time_obj.minute * 60 + time_obj.second

    if bus_id not in bus_data:
        bus_data[bus_id] = {"time": [], "mileage": []}

    bus_data[bus_id]["time"].append(time_in_seconds)
    bus_data[bus_id]["mileage"].append(int(mileage) if mileage else None)

print(f"Bus Data: {bus_data}")

plt.figure(figsize=(10, 6))
for bus_id, data in bus_data.items():
    valid_data = [(t, m) for t, m in zip(data["time"], data["mileage"]) if m is not None]
    if valid_data:
        times, mileages = zip(*valid_data)
        plt.plot(times, mileages, marker="o", label=f"Bus {bus_id}")

plt.title("Bus Time-Space Line Chart")
plt.xlabel("Time (seconds)")
plt.ylabel("Mileage")
plt.legend()
plt.grid(True)

output_path = "bus_time_mileage_chart.png"
plt.savefig(output_path)
print(f"saved plot to {output_path}")

