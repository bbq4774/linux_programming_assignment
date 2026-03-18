import matplotlib.pyplot as plt
import numpy as np

def analyze_and_plot(filename):
    all_cycles = []
    current_cycle = []

    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                if current_cycle:
                    all_cycles.append(np.array(current_cycle))
                    current_cycle = []
            else:
                parts = line.split()
                if len(parts) >= 2:
                    try:
                        interval = int(parts[1])
                        current_cycle.append(interval)
                    except: continue
        if current_cycle: all_cycles.append(np.array(current_cycle))

    x_targets = ["1000000ns", "100000ns", "10000ns", "1000ns", "100ns"]
    
    for i, data in enumerate(all_cycles):
        # Tính toán thống kê trên TOÀN BỘ dữ liệu
        v_max, v_min = np.max(data), np.min(data)
        v_avg        = np.mean(data)

        # Xử lý để "Zoom" vào vùng tập trung (loại bỏ 0.5% đầu và 0.5% cuối)
        # Cách này giúp Histogram hiển thị rõ vùng có 99% dữ liệu
        lower_bound = np.percentile(data, 0.5)
        upper_bound = np.percentile(data, 95)
        filtered_data = data[(data >= lower_bound) & (data <= upper_bound)]

        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
        target_name = x_targets[i] if i < len(x_targets) else f"Cycle {i+1}"
        
        # --- Biểu đồ 1: Line Plot (Vẫn vẽ toàn bộ để thấy Outliers) ---
        ax1.plot(data, color='#1f77b4', alpha=0.7, linewidth=0.5)
        ax1.set_title(f"Toàn bộ mẫu - {target_name}")
        ax1.set_ylabel("Interval (ns)")
        ax1.grid(True, alpha=0.3)

        # --- Biểu đồ 2: Histogram (Chỉ tập trung vùng chính) ---
        # Tăng số lượng bins lên (100) để nhìn rõ sự phân bổ trong vùng zoom
        ax2.hist(filtered_data, bins=100, color='#2ca02c', alpha=0.6, edgecolor='black')
        ax2.set_title(f"Phân bổ tập trung (Zoom 99% data)")
        ax2.set_xlabel("Interval Value (ns)")
        ax2.set_xlim(lower_bound, upper_bound) # Giới hạn trục X vào vùng tập trung
        ax2.grid(True, alpha=0.3)

        # Hiển thị thống kê (vẫn dựa trên data gốc để đảm bảo tính khách quan)
        stats_text = (f"THỐNG KÊ GỐC:\nMax: {v_max}\nMin: {v_min}\nAvg: {v_avg:.1f}\n"
                      f"\nKHU VỰC ZOOM:\nRange: {int(lower_bound)} - {int(upper_bound)} ns")
        
        ax2.text(0.95, 0.95, stats_text, transform=ax2.transAxes, 
                 verticalalignment='top', horizontalalignment='right',
                 bbox=dict(boxstyle='round', facecolor='white', alpha=0.8), fontsize=9)

        plt.tight_layout()
        plt.savefig(f"optimized_cycle_{i+1}.png")
        plt.close()

if __name__ == "__main__":
    analyze_and_plot("time_and_interval.txt")