import re

def calculate_avg_headway(file_path):
    total_deviation = 0
    count = 0
    
    with open(file_path, 'r') as file:
        for line in file:
            match = re.search(r"Avg headway deviation:\s*([\d.]+)", line)
            if match:
                total_deviation += float(match.group(1))
                count += 1
    
    if count == 0:
        print("No 'Avg headway deviation' values found.")
        return None
    
    average = total_deviation / count
    return average

file_path = file_path = "result.txt"
average_deviation = calculate_avg_headway(file_path)

if average_deviation is not None:
    print(f"The overall average of 'Avg headway deviation' is: {average_deviation:.4f}")
