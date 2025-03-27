#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MEMORY_SIZE 100
#define MAX_BLOCKS 20
#define ALGORITHMS 4
#define FRAG_THRESHOLD 5 // Blocks smaller than this are considered fragmented

typedef struct {
    int start;
    int size;
    bool allocated;
} Block;

typedef struct {
    Block memory[MAX_BLOCKS];
    int totalBlocks;
    int lastAlloc;
    int successfulAllocations;
    int failedAllocations;
    int totalRequests;
} MemoryManager;

MemoryManager managers[ALGORITHMS];
const char* algorithmNames[ALGORITHMS] = {"First Fit", "Best Fit", "Worst Fit", "Next Fit"};
void initialize_memory(MemoryManager *m) {
    m->totalBlocks = 1;
    m->lastAlloc = 0;
    m->successfulAllocations = 0;
    m->failedAllocations = 0;
    m->totalRequests = 0;
    m->memory[0].start = 0;
    m->memory[0].size = MEMORY_SIZE;
    m->memory[0].allocated = false;
}

void display_memory(MemoryManager *m, const char* algoName) {
    printf("\n=== %s Memory Layout ===\n", algoName);
    printf("Start End  Size    Status\n");
    printf("----- ---  ----    ------\n");
    for (int i = 0; i < m->totalBlocks; i++) {
        printf("%4d %4d %4d    %s\n",
               m->memory[i].start,
               m->memory[i].start + m->memory[i].size - 1,
               m->memory[i].size,
               m->memory[i].allocated ? "Allocated" : "Free");
    }
}

int first_fit(MemoryManager *m, int size) {
    for (int i = 0; i < m->totalBlocks; i++) {
        if (!m->memory[i].allocated && m->memory[i].size >= size) {
            return i;
        }
    }
    return -1;
}

int best_fit(MemoryManager *m, int size) {
    int bestIndex = -1;
    int minSize = MEMORY_SIZE + 1;
    for (int i = 0; i < m->totalBlocks; i++) {
        if (!m->memory[i].allocated && m->memory[i].size >= size && m->memory[i].size < minSize) {
            bestIndex = i;
            minSize = m->memory[i].size;
        }
    }
    return bestIndex;
}

int worst_fit(MemoryManager *m, int size) {
    int worstIndex = -1;
    int maxSize = -1;
    for (int i = 0; i < m->totalBlocks; i++) {
        if (!m->memory[i].allocated && m->memory[i].size >= size && m->memory[i].size > maxSize) {
            worstIndex = i;
            maxSize = m->memory[i].size;
        }
    }
    return worstIndex;
}

int next_fit(MemoryManager *m, int size) {
    for (int i = m->lastAlloc; i < m->totalBlocks; i++) {
        if (!m->memory[i].allocated && m->memory[i].size >= size) {
            m->lastAlloc = i;
            return i;
        }
    }
    for (int i = 0; i < m->lastAlloc; i++) {
        if (!m->memory[i].allocated && m->memory[i].size >= size) {
            m->lastAlloc = i;
            return i;
        }
    }
    return -1;
}

void allocate_memory(MemoryManager *m, int size, int (*fitFunction)(MemoryManager*, int), const char* algoName) {
    m->totalRequests++;
    int index = fitFunction(m, size);
    
    if (index == -1) {
        printf("  [%s] Failed to allocate %d bytes (no suitable block)\n", algoName, size);
        m->failedAllocations++;
        return;
    }

    // Split block if there's remaining space
    if (m->memory[index].size > size) {
        if (m->totalBlocks >= MAX_BLOCKS) {
            printf("  [%s] Cannot split - maximum blocks reached\n", algoName);
            m->failedAllocations++;
            return;
        }

        // Make space for new block
        for (int i = m->totalBlocks; i > index + 1; i--) {
            m->memory[i] = m->memory[i - 1];
        }

        // Create new free block with remaining space
        m->memory[index + 1].start = m->memory[index].start + size;
        m->memory[index + 1].size = m->memory[index].size - size;
        m->memory[index + 1].allocated = false;
        m->totalBlocks++;
    }

    // Allocate the block
    m->memory[index].size = size;
    m->memory[index].allocated = true;
    m->successfulAllocations++;
    
    printf("  [%s] Allocated %d bytes at %d-%d\n", 
           algoName, size, 
           m->memory[index].start,
           m->memory[index].start + size - 1);
}

void deallocate(MemoryManager *m, int startAddress, const char* algoName) {
    for (int i = 0; i < m->totalBlocks; i++) {
        if (m->memory[i].allocated && m->memory[i].start == startAddress) {
            m->memory[i].allocated = false;
            printf("  [%s] Freed block at %d-%d (%d bytes)\n",
                   algoName,
                   m->memory[i].start,
                   m->memory[i].start + m->memory[i].size - 1,
                   m->memory[i].size);
            
            // Merge with previous free block if possible
            if (i > 0 && !m->memory[i-1].allocated) {
                m->memory[i-1].size += m->memory[i].size;
                for (int j = i; j < m->totalBlocks - 1; j++) {
                    m->memory[j] = m->memory[j + 1];
                }
                m->totalBlocks--;
                i--; // Check the merged block again
            }
            
            // Merge with next free block if possible
            if (i < m->totalBlocks - 1 && !m->memory[i+1].allocated) {
                m->memory[i].size += m->memory[i+1].size;
                for (int j = i + 1; j < m->totalBlocks - 1; j++) {
                    m->memory[j] = m->memory[j + 1];
                }
                m->totalBlocks--;
            }
            return;
        }
    }
    printf("  [%s] No allocated block found at address %d\n", algoName, startAddress);
}

void save_statistics() {
    FILE *file = fopen("memory_stats.txt", "w");
    if (!file) {
        printf("Error opening file!\n");
        return;
    }

    fprintf(file, "Algorithm,Allocated,Free,Fragmentation,SuccessRate\n");

    for (int i = 0; i < ALGORITHMS; i++) {
        int allocated = 0, freeMemory = 0;
        int fragmentedSize = 0;    // Total size of fragmented blocks
        int totalFreeBlocks = 0;  // Count of free blocks

        // Calculate memory usage and fragmentation
        for (int j = 0; j < managers[i].totalBlocks; j++) {
            if (managers[i].memory[j].allocated) {
                allocated += managers[i].memory[j].size;
            } else {
                freeMemory += managers[i].memory[j].size;
                totalFreeBlocks++;
                if (managers[i].memory[j].size <= FRAG_THRESHOLD) {
                    fragmentedSize += managers[i].memory[j].size;
                }
            }
        }

        // Calculate fragmentation percentage
        float fragmentationPercent = 0.0;
        if (freeMemory > 0) {
            // Percentage of free memory that is fragmented
            fragmentationPercent = (fragmentedSize / (float)freeMemory) * 100;
        }

        float successRate = managers[i].totalRequests > 0 
                          ? (managers[i].successfulAllocations / (float)managers[i].totalRequests) * 100 
                          : 0;

        fprintf(file, "%s,%d,%d,%.2f,%.2f\n", 
                algorithmNames[i], allocated, freeMemory, fragmentationPercent, successRate);

        // Debug output
        printf("\n%s Memory Analysis:", algorithmNames[i]);
        printf("\n  Allocated: %d bytes", allocated);
        printf("\n  Free: %d bytes in %d blocks", freeMemory, totalFreeBlocks);
        printf("\n  Fragmented: %d bytes (%.2f%%)", fragmentedSize, fragmentationPercent);
        printf("\n  Success Rate: %.2f%% (%d/%d)", 
               successRate, 
               managers[i].successfulAllocations, 
               managers[i].totalRequests);
    }

    fclose(file);
    printf("\n\nStatistics saved to memory_stats.txt\n");
}

void print_menu() {
    printf("\nMemory Allocation Simulator Menu\n");
    printf("1. Allocate memory\n");
    printf("2. Deallocate memory\n");
    printf("3. Display memory state\n");
    printf("4. Save statistics and exit\n");
    printf("5. Exit without saving\n");
    printf("Enter your choice: ");
}

int main() {
    // Initialize all memory managers
    for (int i = 0; i < ALGORITHMS; i++) {
        initialize_memory(&managers[i]);
    }

    int choice;
    while (1) {
        print_menu();
        scanf("%d", &choice);

        switch (choice) {
            case 1: { // Allocate memory
                int size;
                printf("Enter size to allocate: ");
                scanf("%d", &size);
                
                if (size <= 0 || size > MEMORY_SIZE) {
                    printf("Invalid size! Must be between 1 and %d\n", MEMORY_SIZE);
                    break;
                }

                // Apply allocation to all algorithms
                for (int i = 0; i < ALGORITHMS; i++) {
                    switch (i) {
                        case 0: allocate_memory(&managers[i], size, first_fit, algorithmNames[i]); break;
                        case 1: allocate_memory(&managers[i], size, best_fit, algorithmNames[i]); break;
                        case 2: allocate_memory(&managers[i], size, worst_fit, algorithmNames[i]); break;
                        case 3: allocate_memory(&managers[i], size, next_fit, algorithmNames[i]); break;
                    }
                }
                break;
            }
            case 2: { // Deallocate memory
                int address;
                printf("Enter starting address to free: ");
                scanf("%d", &address);
                
                for (int i = 0; i < ALGORITHMS; i++) {
                    deallocate(&managers[i], address, algorithmNames[i]);
                }
                break;
            }
            case 3: { // Display memory state
                for (int i = 0; i < ALGORITHMS; i++) {
                    display_memory(&managers[i], algorithmNames[i]);
                }
                break;
            }
            case 4: // Save and exit
                save_statistics();
                return 0;
            case 5: // Exit without saving
                return 0;
            default:
                printf("Invalid choice! Please try again.\n");
        }
    }
}