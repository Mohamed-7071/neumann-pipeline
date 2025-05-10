#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MEMORY_SIZE 2048
#define INSTRUCTION_SEGMENT_LIMIT 1024

// Unified instruction + data memory
uint32_t memory[MEMORY_SIZE];

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
    printf("\nInstructions successfully written to instruction memory [0–%d]\n", i - 1);
}

// === Read memory from any address ===
uint32_t read_memory(uint16_t address) {
    if (address >= MEMORY_SIZE) {
        fprintf(stderr, "Memory Read Error: address %d out of bounds.\n", address);
        exit(EXIT_FAILURE);
    }
    return memory[address];
}

// === Write memory to any address ===
void write_memory(uint16_t address, uint32_t value) {
    if (address >= MEMORY_SIZE) {
        fprintf(stderr, "Memory Write Error: address %d out of bounds.\n", address);
        exit(EXIT_FAILURE);
    }
    memory[address] = value;
}

// === Print full memory content ===
void print_memory() {
    printf("\n======= Full Memory Dump =======\n");
    for (int i = 0; i < MEMORY_SIZE; i++) {
        printf("Memory[%4d] = 0x%08X\n", i, memory[i]);
    }
}
