import matplotlib.pyplot as plt

intervals = []

with open("time_and_interval.txt") as f:
    for line in f:
        parts = line.split()
        if len(parts) == 2:
            interval = int(parts[1])
            intervals.append(interval)

plt.plot(intervals)

plt.xlabel("Sample")
plt.ylabel("Interval (ns)")
plt.title("Interval between samples")

plt.show()