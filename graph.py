import matplotlib.pyplot as plt
import numpy as np
import csv

def visualizeResults(): #Function to visualize the output in the form of Graph
    algorithmNames = ["First Fit", "Best Fit", "Worst Fit", "Next Fit"]
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728']

    metrics = {
        'allocated': [],
        'free': [],
        'fragmentation': [],
        'successRate': []
    }

    try:
        with open("memory_stats.txt", "r") as file:
            reader = csv.reader(file)
            next(reader)
            for row in reader:
                if len(row) != 5:
                    continue
                metrics['allocated'].append(int(row[1]))
                metrics['free'].append(int(row[2]))
                metrics['fragmentation'].append(float(row[3]))
                metrics['successRate'].append(float(row[4]))

        if len(metrics['allocated']) != 4:
            raise ValueError("Incomplete data in memory_stats.txt")

        # Adjusted figure size for better readability
        plt.figure(figsize=(9, 6))
        plt.suptitle("Memory Allocation Algorithm Comparison", fontsize=14, y=0.98, fontweight="bold")

        # Comparision of Fragmentation
        ax1 = plt.subplot(2, 2, 1)
        bars = ax1.bar(algorithmNames, metrics['fragmentation'], color=colors)
        ax1.set_title("External Fragmentation", fontsize=11)
        ax1.set_ylabel("Fragmentation (%)")
        ax1.set_ylim(0, 100)
        ax1.grid(axis='y', linestyle='--', alpha=0.7)

        for bar in bars:
            ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 2,
                     f'{bar.get_height():.1f}%', ha='center', fontsize=8)

        # Comparision of success rate
        ax2 = plt.subplot(2, 2, 2)
        bars = ax2.bar(algorithmNames, metrics['successRate'], color=colors)
        ax2.set_title("Allocation Success Rate", fontsize=11)
        ax2.set_ylabel("Success Rate (%)")
        ax2.set_ylim(0, 110)
        ax2.grid(axis='y', linestyle='--', alpha=0.7)

        for bar in bars:
            ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 2,
                     f'{bar.get_height():.1f}%', ha='center', fontsize=8)

        # Usage of Memory
        ax3 = plt.subplot(2, 2, 3)
        width = 0.4
        x = np.arange(len(algorithmNames))

        allocatedBars = ax3.bar(x - width/2, metrics['allocated'], width, label='Allocated', color=colors)
        freeBars = ax3.bar(x + width/2, metrics['free'], width, label='Free', color='#7f7f7f')

        ax3.set_title("Memory Usage", fontsize=11)
        ax3.set_ylabel("Memory (bytes)")
        ax3.set_xticks(x)
        ax3.set_xticklabels(algorithmNames, rotation=10)
        ax3.legend(fontsize=9)
        ax3.grid(axis='y', linestyle='--', alpha=0.7)

        # Score for Combined Performance
        ax4 = plt.subplot(2, 2, 4)
        combinedScore = [(100 - f) * 0.4 + s * 0.6 for f, s in zip(metrics['fragmentation'], metrics['successRate'])]
        bars = ax4.bar(algorithmNames, combinedScore, color=colors)
        
        bestIndex = np.argmax(combinedScore)
        bars[bestIndex].set_color('#9467bd')
        bars[bestIndex].set_edgecolor('black')
        bars[bestIndex].set_linewidth(2)

        ax4.set_title("Performance Score", fontsize=11)
        ax4.set_ylabel("Score (0-100)")
        ax4.set_ylim(0, 110)
        ax4.grid(axis='y', linestyle='--', alpha=0.7)

        for bar in bars:
            ax4.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 2,
                     f'{bar.get_height():.1f}', ha='center', fontsize=8)

        plt.tight_layout()

        print("\n=== Best Performer ===")
        print(f"🏆 {algorithmNames[bestIndex]} - Score: {combinedScore[bestIndex]:.1f}")

        plt.show()

    except FileNotFoundError:
        print("❌ Error: 'memory_stats.txt' not found.")
    except Exception as e:
        print(f"❌ Error: {str(e)}")

if __name__ == "__main__":
    visualizeResults()