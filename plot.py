import matplotlib.pyplot as plt

all_intervals = []
current_intervals = []

with open("time_and_interval.txt") as f:
    for line in f:
        line = line.strip()
        
        if line == "":
            if current_intervals:
                all_intervals.append(current_intervals)
                current_intervals = []
        else:
            parts = line.split()
            if len(parts) == 2:
                interval = int(parts[1])
                current_intervals.append(interval)

if current_intervals:
    all_intervals.append(current_intervals)

# vẽ từng figure
for i, intervals in enumerate(all_intervals):
    plt.figure()
    plt.plot(intervals)
    plt.xlabel("Sample")
    plt.ylabel("Interval (ns)")
    plt.title(f"Cycle {i+1}")

plt.show()