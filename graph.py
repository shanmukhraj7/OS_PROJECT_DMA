import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns

def read_statistics():
    try:
        df = pd.read_csv('memory_stats.txt')
        return df
    except FileNotFoundError:
        print("Error: memory_stats.txt not found. Run the C program first to generate statistics.")
        exit()

def plot_memory_utilization(df):
    plt.figure(figsize=(12, 6))
    ax = sns.barplot(x='Memory Management Technique', y='Allocated', data=df)
    plt.title('Memory Utilization by Technique')
    plt.ylabel('Allocated Memory (bytes)')
    plt.xticks(rotation=45)
    
    # Add values on top of bars
    for p in ax.patches:
        ax.annotate(f"{int(p.get_height())}", 
                    (p.get_x() + p.get_width() / 2., p.get_height()),
                    ha='center', va='center', xytext=(0, 10), textcoords='offset points')
    
    plt.tight_layout()
    plt.savefig('memory_utilization.png')
    plt.show()

def plot_fragmentation(df):
    plt.figure(figsize=(12, 6))
    ax = sns.barplot(x='Memory Management Technique', y='Fragmentation', data=df)
    plt.title('Fragmentation by Technique')
    plt.ylabel('Fragmentation (%)')
    plt.xticks(rotation=45)
    
    # Add values on top of bars
    for p in ax.patches:
        ax.annotate(f"{p.get_height():.1f}%", 
                    (p.get_x() + p.get_width() / 2., p.get_height()),
                    ha='center', va='center', xytext=(0, 10), textcoords='offset points')
    
    plt.tight_layout()
    plt.savefig('fragmentation.png')
    plt.show()

def plot_success_rate(df):
    plt.figure(figsize=(12, 6))
    ax = sns.barplot(x='Memory Management Technique', y='SuccessRate', data=df)
    plt.title('Allocation Success Rate by Technique')
    plt.ylabel('Success Rate (%)')
    plt.xticks(rotation=45)
    
    # Add values on top of bars
    for p in ax.patches:
        ax.annotate(f"{p.get_height():.1f}%", 
                    (p.get_x() + p.get_width() / 2., p.get_height()),
                    ha='center', va='center', xytext=(0, 10), textcoords='offset points')
    
    plt.tight_layout()
    plt.savefig('success_rate.png')
    plt.show()

def plot_comparison(df):
    # Create subplots
    fig, axes = plt.subplots(3, 1, figsize=(12, 15))
    
    # Memory Utilization
    sns.barplot(ax=axes[0], x='Memory Management Technique', y='Allocated', data=df)
    axes[0].set_title('Memory Utilization')
    axes[0].set_ylabel('Allocated (bytes)')
    
    # Fragmentation
    sns.barplot(ax=axes[1], x='Memory Management Technique', y='Fragmentation', data=df)
    axes[1].set_title('Fragmentation')
    axes[1].set_ylabel('Fragmentation (%)')
    
    # Success Rate
    sns.barplot(ax=axes[2], x='Memory Management Technique', y='SuccessRate', data=df)
    axes[2].set_title('Success Rate')
    axes[2].set_ylabel('Success Rate (%)')
    
    # Rotate x-axis labels for all subplots
    for ax in axes:
        for tick in ax.get_xticklabels():
            tick.set_rotation(45)
    
    plt.tight_layout()
    plt.savefig('memory_comparison.png')
    plt.show()

def main():
    # Read the statistics data
    df = read_statistics()
    print("\nMemory Statistics Data:")
    print(df.to_string(index=False))
    
    # Generate visualizations
    print("\nGenerating visualizations...")
    plot_memory_utilization(df)
    plot_fragmentation(df)
    plot_success_rate(df)
    plot_comparison(df)
    print("Visualizations saved as PNG files.")

if __name__ == "__main__":
    main()