import matplotlib.pyplot as plt
import numpy as np
import csv

def visualize_results():
    # Algorithm names and colors for consistency
    algorithmNames = ["First Fit", "Best Fit", "Worst Fit", "Next Fit"]
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728']
    
    # Initialize data lists
    metrics = {
        'allocated': [],
        'free': [],
        'fragmentation': [],
        'success_rate': []
    }

    try:
        # Read data from file
        with open("memory_stats.txt", "r") as file:
            reader = csv.reader(file)
            next(reader)  # Skip header
            
            for row in reader:
                if len(row) != 5:  # Ensure correct number of columns
                    continue
                
                metrics['allocated'].append(int(row[1]))
                metrics['free'].append(int(row[2]))
                metrics['fragmentation'].append(float(row[3]))
                metrics['success_rate'].append(float(row[4]))

        # Verify we have data for all algorithms
        if len(metrics['allocated']) != 4:
            raise ValueError("Incomplete data in memory_stats.txt")

        # Create figure with subplots
        plt.figure(figsize=(14, 10))
        plt.suptitle("Memory Allocation Algorithm Comparison", fontsize=16, y=1.02)

        # Plot 1: Fragmentation Comparison
        ax1 = plt.subplot(2, 2, 1)
        bars = ax1.bar(algorithmNames, metrics['fragmentation'], color=colors)
        ax1.set_title("External Fragmentation Comparison", pad=10)
        ax1.set_ylabel("Fragmentation (%)")
        ax1.set_ylim(0, min(100, max(metrics['fragmentation']) * 1.2))
        ax1.grid(axis='y', linestyle='--', alpha=0.7)
        
        # Add value labels on bars
        for bar in bars:
            height = bar.get_height()
            ax1.text(bar.get_x() + bar.get_width()/2., height + 1,
                    f'{height:.1f}%', ha='center', va='bottom')

        # Plot 2: Success Rate Comparison
        ax2 = plt.subplot(2, 2, 2)
        bars = ax2.bar(algorithmNames, metrics['success_rate'], color=colors)
        ax2.set_title("Allocation Success Rate", pad=10)
        ax2.set_ylabel("Success Rate (%)")
        ax2.set_ylim(0, 110)  # Allow space for value labels
        ax2.grid(axis='y', linestyle='--', alpha=0.7)
        
        for bar in bars:
            height = bar.get_height()
            ax2.text(bar.get_x() + bar.get_width()/2., height + 1,
                    f'{height:.1f}%', ha='center', va='bottom')

        # Plot 3: Memory Usage
        ax3 = plt.subplot(2, 2, 3)
        width = 0.35
        x = np.arange(len(algorithmNames))
        
        allocated_bars = ax3.bar(x - width/2, metrics['allocated'], width, 
                               label='Allocated', color=colors, alpha=0.8)
        free_bars = ax3.bar(x + width/2, metrics['free'], width,
                           label='Free', color='#7f7f7f', alpha=0.8)
        
        ax3.set_title("Memory Usage Distribution", pad=10)
        ax3.set_ylabel("Memory (bytes)")
        ax3.set_xticks(x)
        ax3.set_xticklabels(algorithmNames)
        ax3.legend(loc='upper right')
        ax3.grid(axis='y', linestyle='--', alpha=0.7)

        # Plot 4: Combined Performance Score
        ax4 = plt.subplot(2, 2, 4)
        # Calculate combined score (higher is better)
        combinedScore = []
        for frag, success in zip(metrics['fragmentation'], metrics['success_rate']):
            # Weighted combination (adjust weights as needed)
            combinedScore.append((100 - frag) * 0.4 + success * 0.6)
        
        bars = ax4.bar(algorithmNames, combinedScore, color=colors)
        bestIndex = np.argmax(combinedScore)
        bars[bestIndex].set_color('#9467bd')  # Highlight best performer
        bars[bestIndex].set_edgecolor('black')
        bars[bestIndex].set_linewidth(2)
        
        ax4.set_title("Combined Performance Score\n(Lower Fragmentation + Higher Success Rate)", pad=10)
        ax4.set_ylabel("Score (0-100)")
        ax4.set_ylim(0, 110)
        ax4.grid(axis='y', linestyle='--', alpha=0.7)
        
        for bar in bars:
            height = bar.get_height()
            ax4.text(bar.get_x() + bar.get_width()/2., height + 1,
                    f'{height:.1f}', ha='center', va='bottom')

        # Adjust layout and spacing
        plt.tight_layout()

        # Print analysis results
        print("\n=== Memory Allocation Algorithm Analysis ===")
        print(f"🏆 Best Performer: {algorithmNames[bestIndex]}")
        print(f"   - Fragmentation: {metrics['fragmentation'][bestIndex]:.1f}%")
        print(f"   - Success Rate: {metrics['success_rate'][bestIndex]:.1f}%")
        print(f"   - Combined Score: {combinedScore[bestIndex]:.1f}")
        
        print("\n🔍 Detailed Metrics:")
        for i, name in enumerate(algorithmNames):
            print(f"\n{name}:")
            print(f"  - Allocated: {metrics['allocated'][i]} bytes")
            print(f"  - Free: {metrics['free'][i]} bytes")
            print(f"  - Fragmentation: {metrics['fragmentation'][i]:.1f}%")
            print(f"  - Success Rate: {metrics['success_rate'][i]:.1f}%")

        plt.show()

    except FileNotFoundError:
        print("❌ Error: 'memory_stats.txt' not found. Please run the C program first.")
    except Exception as e:
        print(f"❌ An error occurred: {str(e)}")

if __name__ == "__main__":
    visualize_results()