#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 1000
#define MAX_BLOCKS 20
#define ALGORITHMS 4
#define FRAG_THRESHOLD 5

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

void initializeMemory(MemoryManager *m) {
    m->totalBlocks = 1;
    m->lastAlloc = 0;
    m->successfulAllocations = 0;
    m->failedAllocations = 0;
    m->totalRequests = 0;
    m->memory[0].start = 0;
    m->memory[0].size = MEMORY_SIZE;
    m->memory[0].allocated = false;
}

void displayMemory(MemoryManager *m, const char* algoName) {
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

int firstFit(MemoryManager *m, int size) {
    for (int i = 0; i < m->totalBlocks; i++) {
        if (!m->memory[i].allocated && m->memory[i].size >= size) {
            return i;
        }
    }
    return -1;
}

int bestFit(MemoryManager *m, int size) {
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

int worstFit(MemoryManager *m, int size) {
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

int nextFit(MemoryManager *m, int size) {
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

void allocate(MemoryManager *m, int size, int (*fitFunction)(MemoryManager*, int), const char* algoName) {
    m->totalRequests++;
    int index = fitFunction(m, size);
    
    if (index == -1) {
        printf("  [%s] Failed to allocate %d bytes\n", algoName, size);
        m->failedAllocations++;
        return;
    }

    if (m->memory[index].size > size) {
        if (m->totalBlocks >= MAX_BLOCKS) {
            printf("  [%s] Cannot split - max blocks reached\n", algoName);
            m->failedAllocations++;
            return;
        }

        for (int i = m->totalBlocks; i > index + 1; i--) {
            m->memory[i] = m->memory[i - 1];
        }

        m->memory[index + 1].start = m->memory[index].start + size;
        m->memory[index + 1].size = m->memory[index].size - size;
        m->memory[index + 1].allocated = false;
        m->totalBlocks++;
    }

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
            
            if (i > 0 && !m->memory[i-1].allocated) {
                m->memory[i-1].size += m->memory[i].size;
                for (int j = i; j < m->totalBlocks - 1; j++) {
                    m->memory[j] = m->memory[j + 1];
                }
                m->totalBlocks--;
                i--;
            }
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

void saveStatistics() {
    FILE *file = fopen("memory_stats.txt", "w");
    if (!file) {
        printf("Error opening file!\n");
        return;
    }

    fprintf(file, "Algorithm,Allocated,Free,Fragmentation,SuccessRate\n");

    for (int i = 0; i < ALGORITHMS; i++) {
        int allocated = 0, freeMemory = 0, fragmentedSize = 0, totalFreeBlocks = 0;

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

        float fragmentationPercent = (freeMemory > 0) ? 
                                   (fragmentedSize / (float)freeMemory) * 100 : 0;
        float successRate = managers[i].totalRequests > 0 
                          ? (managers[i].successfulAllocations / (float)managers[i].totalRequests) * 100 
                          : 0;

        fprintf(file, "%s,%d,%d,%.2f,%.2f\n", 
                algorithmNames[i], allocated, freeMemory, fragmentationPercent, successRate);
    }

    fclose(file);
    printf("\nStatistics saved to memory_stats.txt\n");
}

void showCurrentStats() {
    printf("\nCurrent Statistics:\n");
    printf("Algorithm     Success Rate  Fragmentation\n");
    printf("----------    ------------  ------------\n");
    
    for (int i = 0; i < ALGORITHMS; i++) {
        int freeMemory = 0, fragmentedSize = 0;
        
        for (int j = 0; j < managers[i].totalBlocks; j++) {
            if (!managers[i].memory[j].allocated) {
                freeMemory += managers[i].memory[j].size;
                if (managers[i].memory[j].size <= FRAG_THRESHOLD) {
                    fragmentedSize += managers[i].memory[j].size;
                }
            }
        }

        float fragmentation = (freeMemory > 0) ? 
                            (fragmentedSize / (float)freeMemory) * 100 : 0;
        float successRate = managers[i].totalRequests > 0 
                           ? (managers[i].successfulAllocations / (float)managers[i].totalRequests) * 100 
                           : 0;

        printf("%-10s    %6.1f%%       %6.1f%%\n", 
               algorithmNames[i], successRate, fragmentation);
    }
}

void printMenu() {
    printf("\nMemory Allocation Simulator\n");
    printf("1. Allocate memory (all algorithms)\n");
    printf("2. Allocate memory (specific algorithm)\n");
    printf("3. Deallocate memory (all algorithms)\n");
    printf("4. Deallocate memory (specific algorithm)\n");
    printf("5. Display memory state\n");
    printf("6. Show current statistics\n");
    printf("7. Save statistics and generate graphs\n");
    printf("8. Exit\n");
    printf("Choose option: ");
}

void printAlgorithmMenu() {
    printf("\nSelect algorithm:\n");
    for (int i = 0; i < ALGORITHMS; i++) {
        printf("%d. %s\n", i+1, algorithmNames[i]);
    }
    printf("Choose option: ");
}

int main() {
    // Initialize all managers
    for (int i = 0; i < ALGORITHMS; i++) {
        initializeMemory(&managers[i]);
    }

    int choice, algoChoice, size, address;
    while (1) {
        printMenu();
        scanf("%d", &choice);

        switch (choice) {
            case 1: // Allocate in all algorithms
                printf("Enter size to allocate: ");
                scanf("%d", &size);
                if (size <= 0 || size > MEMORY_SIZE) {
                    printf("Invalid size! Must be 1-%d\n", MEMORY_SIZE);
                    break;
                }
                for (int i = 0; i < ALGORITHMS; i++) {
                    switch (i) {
                        case 0: allocate(&managers[i], size, firstFit, algorithmNames[i]); break;
                        case 1: allocate(&managers[i], size, bestFit, algorithmNames[i]); break;
                        case 2: allocate(&managers[i], size, worstFit, algorithmNames[i]); break;
                        case 3: allocate(&managers[i], size, nextFit, algorithmNames[i]); break;
                    }
                }
                break;
                
            case 2: // Allocate in specific algorithm
                printAlgorithmMenu();
                scanf("%d", &algoChoice);
                if (algoChoice < 1 || algoChoice > ALGORITHMS) {
                    printf("Invalid choice!\n");
                    break;
                }
                printf("Enter size to allocate: ");
                scanf("%d", &size);
                if (size <= 0 || size > MEMORY_SIZE) {
                    printf("Invalid size! Must be 1-%d\n", MEMORY_SIZE);
                    break;
                }
                switch (algoChoice-1) {
                    case 0: allocate(&managers[0], size, firstFit, algorithmNames[0]); break;
                    case 1: allocate(&managers[1], size, bestFit, algorithmNames[1]); break;
                    case 2: allocate(&managers[2], size, worstFit, algorithmNames[2]); break;
                    case 3: allocate(&managers[3], size, nextFit, algorithmNames[3]); break;
                }
                break;
                
            case 3: // Deallocate in all algorithms
                printf("Enter starting address to free: ");
                scanf("%d", &address);
                for (int i = 0; i < ALGORITHMS; i++) {
                    deallocate(&managers[i], address, algorithmNames[i]);
                }
                break;
                
            case 4: // Deallocate in specific algorithm
                printAlgorithmMenu();
                scanf("%d", &algoChoice);
                if (algoChoice < 1 || algoChoice > ALGORITHMS) {
                    printf("Invalid choice!\n");
                    break;
                }
                printf("Enter starting address to free: ");
                scanf("%d", &address);
                deallocate(&managers[algoChoice-1], address, algorithmNames[algoChoice-1]);
                break;
                
            case 5: // Display memory
                for (int i = 0; i < ALGORITHMS; i++) {
                    displayMemory(&managers[i], algorithmNames[i]);
                }
                break;
                
            case 6: // Show statistics
                showCurrentStats();
                break;
                
            case 7: // Save and generate graphs
                saveStatistics();
                // Call Python visualization script
                system("python visualize.py");
                break;
                
            case 8: // Exit
                return 0;
                
            default:
                printf("Invalid choice!\n");
        }
    }
}