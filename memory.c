// memory.c
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "memory.h"

#define MEMORY_SIZE 2048
#define INSTRUCTION_SEGMENT_LIMIT 1024

uint32_t memory[MEMORY_SIZE]; // Unified instruction + data memory
uint32_t instruction_memory[MAX_INSTRUCTIONS] = {0};

// In memory.c
uint32_t memory[MEMORY_SIZE] = {0};
// === Initialize memory to 0 ===
void init_memory() {
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = 0;
    }
}

// === Write encoded instruction array into memory segment 0–1023 ===
void write_instruction_memory(uint32_t *instruction_array) {
    int i = 0;
    while (i < INSTRUCTION_SEGMENT_LIMIT && instruction_array[i] != 0) {
        memory[i] = instruction_array[i];
        i++;
    }
    printf("\nInstructions successfully written to instruction memory\n", i - 1);
}

// === Read memory from any address ===
uint32_t read_memory(uint32_t address) {
    if (address >= MEMORY_SIZE) {
        fprintf(stderr, "Memory Read Error: address %d out of bounds.\n", address);
        exit(EXIT_FAILURE);
    }
    return memory[address];
}

void print_memory() {
    printf("\n======= Non-Zero Memory Locations =======\n");
    for (int i = 0; i < MEMORY_SIZE; i++) {
        uint32_t value = memory[i];
        if (value != 0) {
            printf("Memory[%4d] = 0b ", i);
            for (int bit = 31; bit >= 0; bit--) {
                printf("%d", (value >> bit) & 1);
                if (bit % 4 == 0 && bit > 0) {
                    printf(" ");
                }
            }
            printf("\n");
        }
    }
    
           
}
