#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 65536
#define MAX_COMMAND_LENGTH 100

// Structure representing a block of free memory
typedef struct FreeBlock {
    int start;              // Start address of the free block
    int size;               // Size of the free block
    struct FreeBlock* next; // Pointer to the next free block
} FreeBlock;

// Structure representing a process
typedef struct Process {
    int process_id;                 // ID of the process
    struct AllocatedBlock* allocated_blocks; // Pointer to the list of allocated blocks for the process
    struct Process* next;          // Pointer to the next process
} Process;

// Structure representing an allocated block of memory for a process
typedef struct AllocatedBlock {
    int start;                      // Start address of the allocated block
    int size;                       // Size of the allocated block
    struct AllocatedBlock* next;    // Pointer to the next allocated block
} AllocatedBlock;

// Structure representing a node in the waiting queue
typedef struct QueueNode {
    Process* process;               // Pointer to the process
    struct QueueNode* next;        // Pointer to the next node in the queue
} QueueNode;

// Structure representing a waiting queue
typedef struct Queue {
    QueueNode* front;               // Pointer to the front of the queue
    QueueNode* rear;                // Pointer to the rear of the queue
} Queue;

// Global variables
FreeBlock* free_list = NULL;       // Pointer to the head of the free memory block list
Process* process_list = NULL;      // Pointer to the head of the process list
Queue* waiting_queue = NULL;       // Pointer to the waiting queue

// Function declarations
void process_next_command();
FreeBlock* initialize_free_list();
void enqueue(Queue* queue, Process* process);
Process* dequeue(Queue* queue);
int allocate_memory(int process_id, int size);

// Function to initialize the free memory block list
FreeBlock* initialize_free_list() {
    FreeBlock* head = (FreeBlock*)malloc(sizeof(FreeBlock)); // Allocate memory for the head of the free list
    head->start = 0;                // Start address of the free block is 0
    head->size = MEMORY_SIZE;       // Size of the free block is the total memory size
    head->next = NULL;              // Initialize next pointer to NULL (no next block)
    return head;                    // Return the head of the free list
}

// Function to enqueue a process into the waiting queue
void enqueue(Queue* queue, Process* process) {
    QueueNode* new_node = (QueueNode*)malloc(sizeof(QueueNode)); // Allocate memory for a new queue node
    new_node->process = process;    // Set the process of the new node
    new_node->next = NULL;          // Initialize next pointer to NULL
    if (queue->rear == NULL) {      // If queue is empty
        queue->front = queue->rear = new_node; // Set both front and rear to the new node
    } else {
        queue->rear->next = new_node;   // Set the next of the current rear node to the new node
        queue->rear = new_node;         // Update rear to the new node
    }
}

// Function to dequeue a process from the waiting queue
Process* dequeue(Queue* queue) {
    if (queue->front == NULL) return NULL; // If queue is empty, return NULL
    QueueNode* temp = queue->front;         // Store the front node
    Process* process = temp->process;       // Get the process from the front node
    queue->front = queue->front->next;      // Update front to the next node
    if (queue->front == NULL) queue->rear = NULL; // If queue becomes empty, update rear to NULL
    free(temp);                             // Free memory of the dequeued node
    return process;                         // Return the dequeued process
}

// Function to allocate memory for a process
int allocate_memory(int process_id, int size) {
    FreeBlock* current = free_list;    // Initialize current pointer to the head of the free list
    FreeBlock* prev = NULL;            // Initialize previous pointer to NULL

    // Iterate through the free memory blocks
    while (current != NULL) {
        if (current->size >= size) {   // If current block has enough size
            int start_address = current->start; // Get the start address of the allocated block
            current->start += size;             // Update the start address of the free block
            current->size -= size;              // Update the size of the free block

            if (current->size == 0) {           // If the entire block is allocated
                if (prev == NULL) {
                    free_list = current->next;  // Update the head of the free list
                } else {
                    prev->next = current->next; // Update the next pointer of the previous block
                }
                free(current);                  // Free memory of the current block
            }

            // Find the process in the process list
            Process* process = process_list;
            while (process && process->process_id != process_id) {
                process = process->next;
            }
            // If process not found, create a new process
            if (!process) {
                process = (Process*)malloc(sizeof(Process)); // Allocate memory for the new process
                process->process_id = process_id;           // Set process ID
                process->allocated_blocks = NULL;           // Initialize allocated blocks to NULL
                process->next = process_list;               // Add process to the process list
                process_list = process;
            }

            // Allocate memory block for the process
            AllocatedBlock* new_block = (AllocatedBlock*)malloc(sizeof(AllocatedBlock));
            new_block->start = start_address;
            new_block->size = size;
            new_block->next = process->allocated_blocks;
            process->allocated_blocks = new_block;
            printf("Allocated %d bytes to process %d at address %d.\n", size, process_id, new_block->start);
            process_next_command(); // Process next command (simulation purpose)
            return start_address;   // Return the start address of the allocated memory block
        }
        prev = current;                 // Update previous pointer
        current = current->next;        // Move to the next free block
    }
    // If memory allocation fails, enqueue the process
    enqueue(waiting_queue, process);
    return -1; // Memory allocation failed, process is enqueued
}
void show_process_queue() {
    printf("Process Queue:\n"); // Print header for the process queue
    printf("---------------\n"); // Print separator
    QueueNode* current = waiting_queue->front; // Get the front of the waiting queue
    while (current != NULL) { // Iterate through the queue
        printf("Process %d\n", current->process->process_id); // Print the process ID
        current = current->next; // Move to the next node
    }
    printf("---------------\n"); // Print separator
}

void free_memory(int process_id, int address) {
    Process* process = process_list; // Start from the beginning of the process list
    while (process && process->process_id != process_id) { // Find the process with the given ID
        process = process->next;
    }

    if (!process) { // If process not found
        printf("Process %d not found.\n", process_id);
        return;
    }

    AllocatedBlock* block = process->allocated_blocks; // Start from the allocated blocks of the process
    AllocatedBlock* prev_block = NULL;

    while (block && block->start != address) { // Find the block with the given start address
        prev_block = block;
        block = block->next;
    }

    if (!block) { // If block not found
        printf("Address %d not allocated to process %d.\n", address, process_id);
        return;
    }

    // Remove the allocated block
    if (prev_block) {
        prev_block->next = block->next;
    } else {
        process->allocated_blocks = block->next;
    }

    // Create a new free block
    FreeBlock* new_free_block = (FreeBlock*)malloc(sizeof(FreeBlock));
    new_free_block->start = address;
    new_free_block->size = block->size;
    new_free_block->next = free_list;
    free_list = new_free_block;

    free(block); // Free the memory of the block

    // Merge adjacent free blocks
    FreeBlock* current = free_list;
    FreeBlock* next = NULL;

    while (current != NULL) { // Iterate through the free blocks
        next = current->next;
        while (next != NULL && current->start + current->size == next->start) { // Merge if adjacent
            current->size += next->size;
            current->next = next->next;
            free(next);
            next = current->next;
        }
        current = current->next;
    }

    // If all blocks of the process are freed, remove the process from the list
    if (!process->allocated_blocks) {
        Process* temp = process_list;
        Process* prev = NULL;
        while (temp && temp->process_id != process_id) {
            prev = temp;
            temp = temp->next;
        }
        if (prev)
            prev->next = temp->next;
        else
            process_list = temp->next;
        free(temp);
    }

    // Check waiting queue for processes
    Process* waiting_process = dequeue(waiting_queue); // Dequeue a waiting process
    if (waiting_process) { // If a waiting process is dequeued
        printf("Process %d is no longer waiting and is being allocated memory.\n", waiting_process->process_id);
        allocate_memory(waiting_process->process_id, waiting_process->allocated_blocks->size); // Allocate memory for it
    }
}

void show_memory() {
    printf("Memory Status:\n"); // Print header for memory status
    printf("---------------------------------------------------\n"); // Print separator
    printf("| Start Address | End Address   | Status          |\n"); // Print column headers
    printf("---------------------------------------------------\n"); // Print separator

    int current_address = 0; // Initialize current address

    FreeBlock* free_current = free_list; // Start from the beginning of the free block list

    while (current_address < MEMORY_SIZE) { // Iterate through memory addresses
        int found = 0; // Flag to indicate if a block is found at the current address

        if (free_current && free_current->start == current_address) { // If there's a free block at the current address
            printf("| %12d | %12d | Free             |\n", free_current->start, free_current->start + free_current->size - 1); // Print free block
            current_address += free_current->size; // Move to the next address
            free_current = free_current->next; // Move to the next free block
            found = 1; // Set found flag
        } else { // If no free block at the current address
            Process* process = process_list;
            while (process) { // Iterate through processes
                AllocatedBlock* block = process->allocated_blocks;
                while (block) { // Iterate through allocated blocks of each process
                    if (block->start == current_address) { // If allocated block found at the current address
                        printf("| %12d | %12d | Process %5d    |\n", block->start, block->start + block->size - 1, process->process_id); // Print allocated block
                        current_address += block->size; // Move to the next address
                        block = NULL; // Exit loop
                        found = 1; // Set found flag
                    } else {
                        block = block->next; // Move to the next block
                    }
                }
                process = process->next; // Move to the next process
            }
        }

        if (!found) { // If no block found at the current address
            current_address++; // Move to the next address
        }
    }

    printf("---------------------------------------------------\n"); // Print separator
}

void create_process(int process_id) {
    Process* new_process = (Process*)malloc(sizeof(Process)); // Allocate memory for new process
    new_process->process_id = process_id; // Set process ID
    new_process->allocated_blocks = NULL; // Initialize allocated blocks as NULL
    new_process->next = process_list; // Insert new process at the beginning of the process list
    process_list = new_process; // Update process list
    printf("Process %d created.\n", process_id); // Print creation message
}

void terminate_process(int process_id) {
    Process* process = process_list; // Start from the beginning of the process list
    while (process && process->process_id != process_id) { // Find the process with the given ID
        process = process->next;
    }

    if (!process) { // If process not found
        printf("Process %d not found.\n", process_id);
        return;
    }

    // Free memory for all allocated blocks of the process
    AllocatedBlock* block = process->allocated_blocks;
    while (block) {
        free_memory(process_id, block->start);
        block = block->next;
    }

    printf("Process %d terminated.\n", process_id); // Print termination message
}
int main() {
    // Initialize the list of free blocks
    free_list = initialize_free_list();
    // Allocate memory for the waiting queue
    waiting_queue = (Queue*)malloc(sizeof(Queue));
    waiting_queue->front = waiting_queue->rear = NULL;
    // Define a buffer to store the user's command
    char command[MAX_COMMAND_LENGTH];

    // Print welcome message and available commands
    printf("Welcome to Memory Management Simulator!\n");
    printf("Available commands:\n");
    printf("create <process_id>: Create a new process\n");
    printf("terminate <process_id>: Terminate an existing process\n");
    printf("allocate <process_id> <size>: Allocate memory for a process\n");
    printf("free <process_id> <address>: Free memory allocated to a process\n");
    printf("show memory: Display memory status\n");
    printf("exit: Exit the simulator\n");

    // Process user commands in a loop until "exit" is entered
    while (1) {
        process_next_command();
    }

    // Free memory allocated for free blocks
    FreeBlock* current = free_list;
    while (current != NULL) {
        FreeBlock* temp = current;
        current = current->next;
        free(temp);
    }

    // Free memory allocated for processes and their allocated blocks
    Process* process = process_list;
    while (process != NULL) {
        Process* temp = process;
        process = process->next;
        AllocatedBlock* block = temp->allocated_blocks;
        while (block != NULL) {
            AllocatedBlock* temp_block = block;
            block = block->next;
            free(temp_block);
        }
        free(temp);
    }

    // Free memory allocated for the waiting queue
    QueueNode* node = waiting_queue->front;
    while (node != NULL) {
        QueueNode* temp = node;
        node = node->next;
        free(temp);
    }

    // Free memory allocated for the waiting queue structure itself
    free(waiting_queue);

    // Exit the program
    return 0;
}
