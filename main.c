#include <stdio.h>
#include <stdint.h>
#include "pipeline.h"
#include "main.h"

// Mock memory read/write functions
uint32_t read_memory(uint32_t address) {
    // Simulate memory read (for testing purposes)
    return address;
}

void write_memory(uint32_t address, uint32_t value) {
    // Simulate memory write (for testing purposes)
    printf("Memory write at address 0x%08X: value=0x%08X\n", address, value);
}

int main() {
    // Sample instruction memory (encoded instructions)
    uint32_t instruction_memory[] = {
        0x012A4020, // ADD R1, R2, R3 (example encoding)
        0x112A4021, // SUB R1, R2, R3
        0x212A4022, // AND R1, R2, R3
        0x612A0001, // MOVI R1, IMM=1
        0x712A0002, // JEQ R1, R2, IMM=2
        0xB0000003  // JMP ADDRESS=3
    };

    int num_instructions = sizeof(instruction_memory) / sizeof(instruction_memory[0]);

    // Initialize pipeline registers
    uint32_t pc = 0; // Program Counter
    uint32_t if_id_register = 0;
    uint32_t id_ex_register = 0;
    uint32_t ex_mem_register = 0;
    uint32_t mem_wb_register = 0;

    // Simulate pipeline stages
    int cycle = 1;
    while (pc < num_instructions || if_id_register != 0 || id_ex_register != 0 || ex_mem_register != 0 || mem_wb_register != 0) {
        printf("\nCycle %d:\n", cycle);

        // Fetch stage
        if (pc < num_instructions) {
            if_id_register = instruction_fetch(instruction_memory, &pc, &if_id_register, cycle);
        }

        // Decode stage
        if (if_id_register != 0) {
            instruction_decode(if_id_register, &id_ex_register, cycle);
        }

        // Execute stage
        if (id_ex_register != 0) {
            execute(id_ex_register, &ex_mem_register, cycle);
        }

        // Memory access stage (optional for this test)
        if (ex_mem_register != 0) {
            memory_access(ex_mem_register, &mem_wb_register, cycle);
        }

        // Write-back stage
        if (mem_wb_register != 0) {
            write_back(mem_wb_register, cycle);
        }

        // Print pipeline registers
        printf("  PC: %u, IF_ID: 0x%08X, ID_EX: 0x%08X, EX_MEM: 0x%08X, MEM_WB: 0x%08X\n",
               pc, if_id_register, id_ex_register, ex_mem_register, mem_wb_register);

        cycle++;
    }

    return 0;
}