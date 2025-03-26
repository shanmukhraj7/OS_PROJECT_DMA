#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MEMORY_SIZE 100
#define MAX_BLOCKS 20
#define NUM_ALGORITHMS 4
#define FRAG_THRESHOLD 5 // Blocks smaller than this are considered fragmented

typedef struct {
    int start;
    int size;
    bool allocated;
} Block;

typedef struct {
    Block memory[MAX_BLOCKS];
    int total_blocks;
    int last_alloc;
    int successful_allocations;
    int failed_allocations;
    int total_requests;
} MemoryManager;

MemoryManager managers[NUM_ALGORITHMS];
const char* algorithm_names[NUM_ALGORITHMS] = {"First Fit", "Best Fit", "Worst Fit", "Next Fit"};

void initialize_memory(MemoryManager *m) {
    m->total_blocks = 1;
    m->last_alloc = 0;
    m->successful_allocations = 0;
    m->failed_allocations = 0;
    m->total_requests = 0;
    m->memory[0].start = 0;
    m->memory[0].size = MEMORY_SIZE;
    m->memory[0].allocated = false;
}

void display_memory(MemoryManager *m, const char* alg_name) {
    printf("\n=== %s Memory Layout ===\n", alg_name);
    printf("Start End  Size    Status\n");
    printf("----- ---  ----    ------\n");
    for (int i = 0; i < m->total_blocks; i++) {
        printf("%4d %4d %4d    %s\n",
               m->memory[i].start,
               m->memory[i].start + m->memory[i].size - 1,
               m->memory[i].size,
               m->memory[i].allocated ? "Allocated" : "Free");
    }
}

int first_fit(MemoryManager *m, int size) {
    for (int i = 0; i < m->total_blocks; i++) {
        if (!m->memory[i].allocated && m->memory[i].size >= size) {
            return i;
        }
    }
    return -1;
}

int best_fit(MemoryManager *m, int size) {
    int best_index = -1;
    int min_size = MEMORY_SIZE + 1;
    for (int i = 0; i < m->total_blocks; i++) {
        if (!m->memory[i].allocated && m->memory[i].size >= size && m->memory[i].size < min_size) {
            best_index = i;
            min_size = m->memory[i].size;
        }
    }
    return best_index;
}

int worst_fit(MemoryManager *m, int size) {
    int worst_index = -1;
    int max_size = -1;
    for (int i = 0; i < m->total_blocks; i++) {
        if (!m->memory[i].allocated && m->memory[i].size >= size && m->memory[i].size > max_size) {
            worst_index = i;
            max_size = m->memory[i].size;
        }
    }
    return worst_index;
}

int next_fit(MemoryManager *m, int size) {
    for (int i = m->last_alloc; i < m->total_blocks; i++) {
        if (!m->memory[i].allocated && m->memory[i].size >= size) {
            m->last_alloc = i;
            return i;
        }
    }
    for (int i = 0; i < m->last_alloc; i++) {
        if (!m->memory[i].allocated && m->memory[i].size >= size) {
            m->last_alloc = i;
            return i;
        }
    }
    return -1;
}

void allocate_memory(MemoryManager *m, int size, int (*fit_function)(MemoryManager*, int), const char* alg_name) {
    m->total_requests++;
    int index = fit_function(m, size);
    
    if (index == -1) {
        printf("  [%s] Failed to allocate %d bytes (no suitable block)\n", alg_name, size);
        m->failed_allocations++;
        return;
    }

    // Split block if there's remaining space
    if (m->memory[index].size > size) {
        if (m->total_blocks >= MAX_BLOCKS) {
            printf("  [%s] Cannot split - maximum blocks reached\n", alg_name);
            m->failed_allocations++;
            return;
        }

        // Make space for new block
        for (int i = m->total_blocks; i > index + 1; i--) {
            m->memory[i] = m->memory[i - 1];
        }

        // Create new free block with remaining space
        m->memory[index + 1].start = m->memory[index].start + size;
        m->memory[index + 1].size = m->memory[index].size - size;
        m->memory[index + 1].allocated = false;
        m->total_blocks++;
    }

    // Allocate the block
    m->memory[index].size = size;
    m->memory[index].allocated = true;
    m->successful_allocations++;
    
    printf("  [%s] Allocated %d bytes at %d-%d\n", 
           alg_name, size, 
           m->memory[index].start,
           m->memory[index].start + size - 1);
}

void deallocate(MemoryManager *m, int start_addr, const char* alg_name) {
    for (int i = 0; i < m->total_blocks; i++) {
        if (m->memory[i].allocated && m->memory[i].start == start_addr) {
            m->memory[i].allocated = false;
            printf("  [%s] Freed block at %d-%d (%d bytes)\n",
                   alg_name,
                   m->memory[i].start,
                   m->memory[i].start + m->memory[i].size - 1,
                   m->memory[i].size);
            
            // Merge with previous free block if possible
            if (i > 0 && !m->memory[i-1].allocated) {
                m->memory[i-1].size += m->memory[i].size;
                for (int j = i; j < m->total_blocks - 1; j++) {
                    m->memory[j] = m->memory[j + 1];
                }
                m->total_blocks--;
                i--; // Check the merged block again
            }
            
            // Merge with next free block if possible
            if (i < m->total_blocks - 1 && !m->memory[i+1].allocated) {
                m->memory[i].size += m->memory[i+1].size;
                for (int j = i + 1; j < m->total_blocks - 1; j++) {
                    m->memory[j] = m->memory[j + 1];
                }
                m->total_blocks--;
            }
            return;
        }
    }
    printf("  [%s] No allocated block found at address %d\n", alg_name, start_addr);
}

void save_statistics() {
    FILE *file = fopen("memory_stats.txt", "w");
    if (!file) {
        printf("Error opening file!\n");
        return;
    }

    fprintf(file, "Algorithm,Allocated,Free,Fragmentation,SuccessRate\n");

    for (int i = 0; i < NUM_ALGORITHMS; i++) {
        int allocated = 0, free_mem = 0;
        int fragmented_size = 0;    // Total size of fragmented blocks
        int total_free_blocks = 0;  // Count of free blocks

        // Calculate memory usage and fragmentation
        for (int j = 0; j < managers[i].total_blocks; j++) {
            if (managers[i].memory[j].allocated) {
                allocated += managers[i].memory[j].size;
            } else {
                free_mem += managers[i].memory[j].size;
                total_free_blocks++;
                if (managers[i].memory[j].size <= FRAG_THRESHOLD) {
                    fragmented_size += managers[i].memory[j].size;
                }
            }
        }

        // Calculate fragmentation percentage
        float fragmentation_percent = 0.0;
        if (free_mem > 0) {
            // Percentage of free memory that is fragmented
            fragmentation_percent = (fragmented_size / (float)free_mem) * 100;
        }

        float success_rate = managers[i].total_requests > 0 
                          ? (managers[i].successful_allocations / (float)managers[i].total_requests) * 100 
                          : 0;

        fprintf(file, "%s,%d,%d,%.2f,%.2f\n", 
                algorithm_names[i], allocated, free_mem, fragmentation_percent, success_rate);

        // Debug output
        printf("\n%s Memory Analysis:", algorithm_names[i]);
        printf("\n  Allocated: %d bytes", allocated);
        printf("\n  Free: %d bytes in %d blocks", free_mem, total_free_blocks);
        printf("\n  Fragmented: %d bytes (%.2f%%)", fragmented_size, fragmentation_percent);
        printf("\n  Success Rate: %.2f%% (%d/%d)", 
               success_rate, 
               managers[i].successful_allocations, 
               managers[i].total_requests);
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
    for (int i = 0; i < NUM_ALGORITHMS; i++) {
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
                for (int i = 0; i < NUM_ALGORITHMS; i++) {
                    switch (i) {
                        case 0: allocate_memory(&managers[i], size, first_fit, algorithm_names[i]); break;
                        case 1: allocate_memory(&managers[i], size, best_fit, algorithm_names[i]); break;
                        case 2: allocate_memory(&managers[i], size, worst_fit, algorithm_names[i]); break;
                        case 3: allocate_memory(&managers[i], size, next_fit, algorithm_names[i]); break;
                    }
                }
                break;
            }
            case 2: { // Deallocate memory
                int addr;
                printf("Enter starting address to free: ");
                scanf("%d", &addr);
                
                for (int i = 0; i < NUM_ALGORITHMS; i++) {
                    deallocate(&managers[i], addr, algorithm_names[i]);
                }
                break;
            }
            case 3: { // Display memory state
                for (int i = 0; i < NUM_ALGORITHMS; i++) {
                    display_memory(&managers[i], algorithm_names[i]);
                }
                break;
            }
            case 4: // Save and exit
                save_statistics();
                return 0;
            case 5: // Exit without saving
                return 0;
            default:
                printf("Invalid choice! Please try again.\n"); // Default case
        }
    }
}