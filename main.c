#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 1000
#define MAX_BLOCKS 20
#define ALGORITHMS 4
#define FRAG_THRESHOLD 5
#define PAGE_SIZE 50
#define MAX_PROCESSES 10

typedef struct {
    int start;
    int size;
    bool allocated;
    int processID;
} Block;

typedef struct {
    Block memory[MAX_BLOCKS];
    int totalBlocks;
    int lastAlloc;
    int successfulAllocations;
    int failedAllocations;
    int totalRequests;
} MemoryManager;

typedef struct {
    int pageTable[20]; // Assuming max 20 pages per process
    int size;
    int processID;
} Process;

MemoryManager managers[ALGORITHMS];
const char* algorithmNames[ALGORITHMS] = {"First Fit", "Best Fit", "Worst Fit", "Next Fit"};
Process processes[MAX_PROCESSES];
int nextProcessID = 1;
int pageFrames[MEMORY_SIZE/PAGE_SIZE]; // Tracks which frames are allocated

void initializeMemory(MemoryManager *m) {
    m->totalBlocks = 1;
    m->lastAlloc = 0;
    m->successfulAllocations = 0;
    m->failedAllocations = 0;
    m->totalRequests = 0;
    m->memory[0].start = 0;
    m->memory[0].size = MEMORY_SIZE;
    m->memory[0].allocated = false;
    m->memory[0].processID = -1;
}

void initializePaging() {
    for (int i = 0; i < MEMORY_SIZE/PAGE_SIZE; i++) {
        pageFrames[i] = -1; // -1 means frame is free
    }
}

void displayMemory(MemoryManager *m, const char* algoName) {
    printf("\n=== %s Memory Layout ===\n", algoName);
    printf("Start End  Size    Status      Process\n");
    printf("----- ---  ----    ------      -------\n");
    for (int i = 0; i < m->totalBlocks; i++) {
        printf("%4d %4d %4d    %-10s %d\n",
               m->memory[i].start,
               m->memory[i].start + m->memory[i].size - 1,
               m->memory[i].size,
               m->memory[i].allocated ? "Allocated" : "Free",
               m->memory[i].processID);
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

void allocate(MemoryManager *m, int size, int (*fitFunction)(MemoryManager*, int), const char* algoName, int processID) {
    m->totalRequests++;
    int index = fitFunction(m, size);
    
    if (index == -1) {
        printf("  [%s] Failed to allocate %d bytes for process %d\n", algoName, size, processID);
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
        m->memory[index + 1].processID = -1;
        m->totalBlocks++;
    }

    m->memory[index].size = size;
    m->memory[index].allocated = true;
    m->memory[index].processID = processID;
    m->successfulAllocations++;
    
    printf("  [%s] Allocated %d bytes at %d-%d for process %d\n", 
           algoName, size, 
           m->memory[index].start,
           m->memory[index].start + size - 1,
           processID);
}

void deallocate(MemoryManager *m, int processID, const char* algoName) {
    bool found = false;
    for (int i = 0; i < m->totalBlocks; i++) {
        if (m->memory[i].allocated && m->memory[i].processID == processID) {
            m->memory[i].allocated = false;
            m->memory[i].processID = -1;
            printf("  [%s] Freed block at %d-%d (%d bytes) for process %d\n",
                   algoName,
                   m->memory[i].start,
                   m->memory[i].start + m->memory[i].size - 1,
                   m->memory[i].size,
                   processID);
            found = true;
            
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
        }
    }
    if (!found) {
        printf("  [%s] No allocated blocks found for process %d\n", algoName, processID);
    }
}

void allocatePages(int processID, int size) {
    int pagesNeeded = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    int allocatedPages = 0;
    
    printf("\nAttempting to allocate %d pages for process %d\n", pagesNeeded, processID);
    
    for (int i = 0; i < MEMORY_SIZE/PAGE_SIZE && allocatedPages < pagesNeeded; i++) {
        if (pageFrames[i] == -1) {
            pageFrames[i] = processID;
            processes[processID].pageTable[allocatedPages] = i;
            allocatedPages++;
            printf("  Allocated page %d (frame %d) to process %d\n", 
                   allocatedPages-1, i, processID);
        }
    }
    
    if (allocatedPages < pagesNeeded) {
        printf("  Could only allocate %d of %d needed pages\n", allocatedPages, pagesNeeded);
        // Free any allocated pages if we couldn't satisfy the request
        for (int i = 0; i < allocatedPages; i++) {
            pageFrames[processes[processID].pageTable[i]] = -1;
        }
    } else {
        processes[processID].size = size;
        processes[processID].processID = processID;
        printf("  Successfully allocated %d pages for process %d\n", pagesNeeded, processID);
    }
}

void deallocatePages(int processID) {
    printf("\nDeallocating pages for process %d\n", processID);
    for (int i = 0; i < MEMORY_SIZE/PAGE_SIZE; i++) {
        if (pageFrames[i] == processID) {
            pageFrames[i] = -1;
            printf("  Freed frame %d from process %d\n", i, processID);
        }
    }
    processes[processID].size = 0;
}

void displayPageTable(int processID) {
    if (processes[processID].size == 0) {
        printf("No pages allocated for process %d\n", processID);
        return;
    }
    
    printf("\nPage Table for Process %d:\n", processID);
    printf("Page  Frame\n");
    printf("----  -----\n");
    
    int pages = (processes[processID].size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (int i = 0; i < pages; i++) {
        printf("%4d  %5d\n", i, processes[processID].pageTable[i]);
    }
}

void displayPagingMemory() {
    printf("\nPaging Memory Status (Frame Allocation):\n");
    printf("Frame  Process\n");
    printf("-----  -------\n");
    for (int i = 0; i < MEMORY_SIZE/PAGE_SIZE; i++) {
        printf("%5d  %7d\n", i, pageFrames[i]);
    }
}

void allocateSegment(int processID, int size) {
    printf("\nAllocating segment for process %d using First Fit\n", processID);
    allocate(&managers[0], size, firstFit, "Segmentation", processID);
}

void deallocateSegment(int processID) {
    printf("\nDeallocating segments for process %d\n", processID);
    deallocate(&managers[0], processID, "Segmentation");
}

void displaySegments(int processID) {
    printf("\nSegments for Process %d:\n", processID);
    printf("Start  Size\n");
    printf("-----  ----\n");
    for (int i = 0; i < managers[0].totalBlocks; i++) {
        if (managers[0].memory[i].allocated && managers[0].memory[i].processID == processID) {
            printf("%5d  %4d\n", managers[0].memory[i].start, managers[0].memory[i].size);
        }
    }
}

void saveStatistics() {
    FILE *file = fopen("memory_stats.txt", "w");
    if (!file) {
        printf("Error opening file!\n");
        return;
    }

    fprintf(file, "Memory Management Technique,Allocated,Free,Fragmentation,SuccessRate,ExtraInfo\n");

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

        fprintf(file, "%s,%d,%d,%.2f,%.2f,%s\n", 
                algorithmNames[i], allocated, freeMemory, fragmentationPercent, successRate,
                "Dynamic Partitioning");
    }

    int allocatedFrames = 0;
    int totalFrames = MEMORY_SIZE/PAGE_SIZE;
    for (int i = 0; i < totalFrames; i++) {
        if (pageFrames[i] != -1) {
            allocatedFrames++;
        }
    }
    float pagingUtilization = (allocatedFrames / (float)totalFrames) * 100;
    fprintf(file, "Paging,%d,%d,%.2f,%.2f,%s\n",
            allocatedFrames*PAGE_SIZE,
            (totalFrames-allocatedFrames)*PAGE_SIZE,
            0.0,
            100.0,
            "Frame Utilization");

    int segmentAllocated = 0, segmentFree = 0, segmentFragments = 0;
    for (int j = 0; j < managers[0].totalBlocks; j++) {
        if (managers[0].memory[j].allocated) {
            segmentAllocated += managers[0].memory[j].size;
        } else {
            segmentFree += managers[0].memory[j].size;
            if (managers[0].memory[j].size <= FRAG_THRESHOLD) {
                segmentFragments++;
            }
        }
    }
    float segmentFragmentation = (segmentFree > 0) ? 
                           (segmentFragments * FRAG_THRESHOLD / (float)segmentFree) * 100 : 0;
    float segmentSuccessRate = managers[0].totalRequests > 0 
                         ? (managers[0].successfulAllocations / (float)managers[0].totalRequests) * 100 
                         : 0;
    fprintf(file, "Segmentation,%d,%d,%.2f,%.2f,%s\n",
            segmentAllocated, segmentFree, segmentFragmentation, segmentSuccessRate,
            "External Fragmentation");

    fclose(file);
    printf("\nStatistics saved to memory_stats.txt\n");
}

void showCurrentStats() {
    printf("\nCurrent Statistics:\n");
    printf("Technique           Allocated  Free     Fragmentation  Success\n");
    printf("------------------  ---------  -------  ------------  -------\n");
    
    for (int i = 0; i < ALGORITHMS; i++) {
        int allocated = 0, freeMemory = 0, fragmentedSize = 0;
        
        for (int j = 0; j < managers[i].totalBlocks; j++) {
            if (managers[i].memory[j].allocated) {
                allocated += managers[i].memory[j].size;
            } else {
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

        printf("%-18s  %6d    %6d    %6.1f%%       %5.1f%%\n", 
               algorithmNames[i], allocated, freeMemory, fragmentation, successRate);
    }

    int allocatedFrames = 0;
    int totalFrames = MEMORY_SIZE/PAGE_SIZE;
    for (int i = 0; i < totalFrames; i++) {
        if (pageFrames[i] != -1) {
            allocatedFrames++;
        }
    }
    printf("%-18s  %6d    %6d    %6.1f%%       %5.1f%%\n",
           "Paging", 
           allocatedFrames*PAGE_SIZE,
           (totalFrames-allocatedFrames)*PAGE_SIZE,
           0.0,
           100.0);

    int segmentAllocated = 0, segmentFree = 0, segmentFragments = 0;
    for (int j = 0; j < managers[0].totalBlocks; j++) {
        if (managers[0].memory[j].allocated) {
            segmentAllocated += managers[0].memory[j].size;
        } else {
            segmentFree += managers[0].memory[j].size;
            if (managers[0].memory[j].size <= FRAG_THRESHOLD) {
                segmentFragments++;
            }
        }
    }
    float segmentFragmentation = (segmentFree > 0) ? 
                           (segmentFragments * FRAG_THRESHOLD / (float)segmentFree) * 100 : 0;
    float segmentSuccessRate = managers[0].totalRequests > 0 
                         ? (managers[0].successfulAllocations / (float)managers[0].totalRequests) * 100 
                         : 0;
    printf("%-18s  %6d    %6d    %6.1f%%       %5.1f%%\n",
           "Segmentation",
           segmentAllocated, segmentFree, segmentFragmentation, segmentSuccessRate);
}

void printMainMenu() {
    printf("\nMemory Management Simulator\n");
    printf("1. Dynamic Partitioning\n");
    printf("2. Paging\n");
    printf("3. Segmentation\n");
    printf("4. View Statistics\n");
    printf("5. Save Statistics\n");
    printf("6. Exit\n");
    printf("Choose option: ");
}

void printDynamicPartitionMenu() {
    printf("\nDynamic Partitioning Algorithms\n");
    printf("1. Allocate memory (all algorithms)\n");
    printf("2. Allocate memory (specific algorithm)\n");
    printf("3. Deallocate memory (all algorithms)\n");
    printf("4. Deallocate memory (specific algorithm)\n");
    printf("5. Display memory state\n");
    printf("6. Back to main menu\n");
    printf("Choose option: ");
}

void printAlgorithmMenu() {
    printf("\nSelect algorithm:\n");
    for (int i = 0; i < ALGORITHMS; i++) {
        printf("%d. %s\n", i+1, algorithmNames[i]);
    }
    printf("Choose option: ");
}

void printPagingMenu() {
    printf("\nPaging System\n");
    printf("1. Create new process\n");
    printf("2. Allocate pages to process\n");
    printf("3. Deallocate process pages\n");
    printf("4. Display page table\n");
    printf("5. Display frame allocation\n");
    printf("6. Back to main menu\n");
    printf("Choose option: ");
}

void printSegmentationMenu() {
    printf("\nSegmentation System\n");
    printf("1. Create new process\n");
    printf("2. Allocate segment to process\n");
    printf("3. Deallocate process segments\n");
    printf("4. Display segments\n");
    printf("5. Back to main menu\n");
    printf("Choose option: ");
}

int main() {
    // Initialize all managers
    for (int i = 0; i < ALGORITHMS; i++) {
        initializeMemory(&managers[i]);
    }
    initializePaging();
    
    int mainChoice, subChoice, algoChoice, size, processID;
    while (1) {
        printMainMenu();
        scanf("%d", &mainChoice);

        switch (mainChoice) {
            case 1: // Dynamic Partitioning
                while (1) {
                    printDynamicPartitionMenu();
                    scanf("%d", &subChoice);
                    
                    if (subChoice == 6) break;
                    
                    switch (subChoice) {
                        case 1: // Allocate in all algorithms
                            printf("Enter process ID: ");
                            scanf("%d", &processID);
                            printf("Enter size to allocate: ");
                            scanf("%d", &size);
                            if (size <= 0 || size > MEMORY_SIZE) {
                                printf("Invalid size! Must be 1-%d\n", MEMORY_SIZE);
                                break;
                            }
                            for (int i = 0; i < ALGORITHMS; i++) {
                                switch (i) {
                                    case 0: allocate(&managers[i], size, firstFit, algorithmNames[i], processID); break;
                                    case 1: allocate(&managers[i], size, bestFit, algorithmNames[i], processID); break;
                                    case 2: allocate(&managers[i], size, worstFit, algorithmNames[i], processID); break;
                                    case 3: allocate(&managers[i], size, nextFit, algorithmNames[i], processID); break;
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
                            printf("Enter process ID: ");
                            scanf("%d", &processID);
                            printf("Enter size to allocate: ");
                            scanf("%d", &size);
                            if (size <= 0 || size > MEMORY_SIZE) {
                                printf("Invalid size! Must be 1-%d\n", MEMORY_SIZE);
                                break;
                            }
                            switch (algoChoice-1) {
                                case 0: allocate(&managers[0], size, firstFit, algorithmNames[0], processID); break;
                                case 1: allocate(&managers[1], size, bestFit, algorithmNames[1], processID); break;
                                case 2: allocate(&managers[2], size, worstFit, algorithmNames[2], processID); break;
                                case 3: allocate(&managers[3], size, nextFit, algorithmNames[3], processID); break;
                            }
                            break;
                            
                        case 3: // Deallocate in all algorithms
                            printf("Enter process ID to deallocate: ");
                            scanf("%d", &processID);
                            for (int i = 0; i < ALGORITHMS; i++) {
                                deallocate(&managers[i], processID, algorithmNames[i]);
                            }
                            break;
                            
                        case 4: // Deallocate in specific algorithm
                            printAlgorithmMenu();
                            scanf("%d", &algoChoice);
                            if (algoChoice < 1 || algoChoice > ALGORITHMS) {
                                printf("Invalid choice!\n");
                                break;
                            }
                            printf("Enter process ID to deallocate: ");
                            scanf("%d", &processID);
                            deallocate(&managers[algoChoice-1], processID, algorithmNames[algoChoice-1]);
                            break;
                            
                        case 5: // Display memory
                            for (int i = 0; i < ALGORITHMS; i++) {
                                displayMemory(&managers[i], algorithmNames[i]);
                            }
                            break;
                            
                        default:
                            printf("Invalid choice!\n");
                    }
                }
                break;
                
            case 2: // Paging
                while (1) {
                    printPagingMenu();
                    scanf("%d", &subChoice);
                    
                    if (subChoice == 6) break;
                    
                    switch (subChoice) {
                        case 1: // Create new process
                            if (nextProcessID >= MAX_PROCESSES) {
                                printf("Maximum number of processes reached!\n");
                                break;
                            }
                            processes[nextProcessID].processID = nextProcessID;
                            processes[nextProcessID].size = 0;
                            printf("Created new process with ID: %d\n", nextProcessID);
                            nextProcessID++;
                            break;
                            
                        case 2: // Allocate pages
                            printf("Enter process ID: ");
                            scanf("%d", &processID);
                            if (processID <= 0 || processID >= nextProcessID) {
                                printf("Invalid process ID!\n");
                                break;
                            }
                            printf("Enter size to allocate: ");
                            scanf("%d", &size);
                            if (size <= 0 || size > MEMORY_SIZE) {
                                printf("Invalid size! Must be 1-%d\n", MEMORY_SIZE);
                                break;
                            }
                            allocatePages(processID, size);
                            break;
                            
                        case 3: // Deallocate pages
                            printf("Enter process ID: ");
                            scanf("%d", &processID);
                            if (processID <= 0 || processID >= nextProcessID) {
                                printf("Invalid process ID!\n");
                                break;
                            }
                            deallocatePages(processID);
                            break;
                            
                        case 4: // Display page table
                            printf("Enter process ID: ");
                            scanf("%d", &processID);
                            if (processID <= 0 || processID >= nextProcessID) {
                                printf("Invalid process ID!\n");
                                break;
                            }
                            displayPageTable(processID);
                            break;
                            
                        case 5: // Display frame allocation
                            displayPagingMemory();
                            break;
                            
                        default:
                            printf("Invalid choice!\n");
                    }
                }
                break;
                
            case 3: // Segmentation
                while (1) {
                    printSegmentationMenu();
                    scanf("%d", &subChoice);
                    
                    if (subChoice == 5) break;
                    
                    switch (subChoice) {
                        case 1: // Create new process
                            if (nextProcessID >= MAX_PROCESSES) {
                                printf("Maximum number of processes reached!\n");
                                break;
                            }
                            printf("Created new process with ID: %d\n", nextProcessID);
                            nextProcessID++;
                            break;
                            
                        case 2: // Allocate segment
                            printf("Enter process ID: ");
                            scanf("%d", &processID);
                            if (processID <= 0 || processID >= nextProcessID) {
                                printf("Invalid process ID!\n");
                                break;
                            }
                            printf("Enter size to allocate: ");
                            scanf("%d", &size);
                            if (size <= 0 || size > MEMORY_SIZE) {
                                printf("Invalid size! Must be 1-%d\n", MEMORY_SIZE);
                                break;
                            }
                            allocateSegment(processID, size);
                            break;
                            
                        case 3: // Deallocate segments
                            printf("Enter process ID: ");
                            scanf("%d", &processID);
                            if (processID <= 0 || processID >= nextProcessID) {
                                printf("Invalid process ID!\n");
                                break;
                            }
                            deallocateSegment(processID);
                            break;
                            
                        case 4: // Display segments
                            printf("Enter process ID: ");
                            scanf("%d", &processID);
                            if (processID <= 0 || processID >= nextProcessID) {
                                printf("Invalid process ID!\n");
                                break;
                            }
                            displaySegments(processID);
                            break;
                            
                        default:
                            printf("Invalid choice!\n");
                    }
                }
                break;
                
            case 4: // View Statistics
                showCurrentStats();
                break;
                
            case 5: // Save Statistics
                saveStatistics();
                break;
                
            case 6: // Exit
                saveStatistics();
                return 0;
                
            default:
                printf("Invalid choice!\n");
        }
    }
}