// funcs.c
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "registers.h"
#include "memory.h"
# include "funcs.h"
#include "parser.h"

#define FILENAME "program.txt"
#define MAX_INSTRUCTIONS 1024

extern uint32_t instruction_memory[MAX_INSTRUCTIONS];
bool flagwork = true;

uint32_t instruction = 0;
int flush_flag = 0;
uint32_t branch_target = 0;

// Globals for MOVR flow
uint32_t finalResult = 0;

extern uint32_t PC;
extern uint32_t registers[NUM_REGISTERS];


// Safe register write: R0 is protected
void safe_register_write(uint8_t reg, uint32_t value, const char* stage) {
    if (reg == 0) {
        registers[0] = 0;
        printf("%s: Attempted to write to R0 — skipped (R0 remains 0)\n", stage);
    } else if (reg < NUM_REGISTERS) {
        registers[reg] = value;
        printf("%s: Wrote %u to R%d\n", stage, value, reg);
    } else {
        fprintf(stderr, "%s: Invalid register index R%d\n", stage, reg);
    }
}


// Instruction Fetch
uint32_t instruction_fetch(uint32_t *memory) {
    if(memory[PC]==0){

        flagwork=false;

    }


    if (PC > 1023) {
        fprintf(stderr, "Fetch Error: PC (%u) out of instruction memory range [0–1023].\n", PC);
        return 0;
    }

    uint32_t instr = memory[PC];
    printf("Fetch Stage: Fetched instruction at address %u => 0x%08X\n", PC, instr);
    return instr;


}


// Instruction Decode
void instruction_decode(uint32_t binary_instruction, Instruction *instr) {
    uint8_t opcodee = (binary_instruction >> 28) & 0xF;
    instr->opcode = opcodee;

    if (opcodee >= 0 && opcodee <= 5) {
        // R-Type
        instr->r1 = (binary_instruction >> 23) & 0x1F; // destination
        instr->r2 = (binary_instruction >> 18) & 0x1F; // source 1
        instr->r3 = (binary_instruction >> 13) & 0x1F; // source 2
        instr->shamt = binary_instruction & 0x1FFF;
        instr->imm = 0;
        instr->addr = 0;
    } else if (opcodee == 6) {
        // MOVI (I-Type): r1 = dest, imm = value
        instr->r1 = (binary_instruction >> 23) & 0x1F;
        instr->r2 = 0;
        instr->imm = binary_instruction & 0x3FFFF;
        if (instr->imm & (1 << 17)) instr->imm |= ~0x3FFFF;
        instr->r3 = 0;
        instr->shamt = 0;
        instr->addr = 0;
    } else if (opcodee == 7) {
        // JEQ (I-Type): r1, r2, imm
        instr->r1 = (binary_instruction >> 23) & 0x1F;
        instr->r2 = (binary_instruction >> 18) & 0x1F;
        instr->imm = binary_instruction & 0x3FFFF;
        if (instr->imm & (1 << 17)) instr->imm |= ~0x3FFFF;
        instr->r3 = 0;
        instr->shamt = 0;
        instr->addr = 0;
    } else if (opcodee == 8) {
        // XORI (I-Type): r1 = dest, imm = value
        instr->r1 = (binary_instruction >> 23) & 0x1F;
        instr->r2 = 0;
        instr->imm = binary_instruction & 0x3FFFF;
        if (instr->imm & (1 << 17)) instr->imm |= ~0x3FFFF;
        instr->r3 = 0;
        instr->shamt = 0;
        instr->addr = 0;
    } else if (opcodee == 9 || opcodee == 10) {
        // MOVR/MOVM (I-Type): r1 = dest/src, r2 = addr base, imm = offset
        instr->r1 = (binary_instruction >> 23) & 0x1F;
        instr->r2 = (binary_instruction >> 18) & 0x1F;
        instr->imm = binary_instruction & 0x3FFFF;
        if (instr->imm & (1 << 17)) instr->imm |= ~0x3FFFF;
        instr->r3 = 0;
        instr->shamt = 0;
        instr->addr = 0;
    } else if (opcodee == 11) {
        // J-Type
        instr->addr = binary_instruction & 0x0FFFFFFF;
        instr->r1 = instr->r2 = instr->r3 = 0;
        instr->shamt = instr->imm = 0;
    } else {
        fprintf(stderr, "Invalid opcode: %d\n", opcodee);
        exit(EXIT_FAILURE);
    }
}


// Execute Stage
uint32_t execute(Instruction *instr, uint32_t instruction_address) {
    int32_t result = 0;
    switch (instr->opcode) {
        case 0: // ADD
            result = (int32_t)registers[instr->r2] + (int32_t)registers[instr->r3];
            break;
        case 1: // SUB
            result = (int32_t)registers[instr->r2] - (int32_t)registers[instr->r3];
            break;
        case 2: // MUL
            result = (int32_t)registers[instr->r2] * (int32_t)registers[instr->r3];
            break;
        case 3: // AND
            result = registers[instr->r2] & registers[instr->r3];
            break;
        case 4: // LSL
            result = registers[instr->r2] << (instr->shamt & 0x1FFF);
            break;
        case 5: // LSR
            result = (uint32_t)registers[instr->r2] >> (instr->shamt & 0x1FFF);
            break;
        case 6: // MOVI
            result = instr->imm;
            break;
        case 7: // JEQ
            if (registers[instr->r1] == registers[instr->r2]) {
                flush_flag = 1;
                branch_target = instruction_address + instr->imm;
                printf("JEQ: Branch taken. New PC will be %u\n", branch_target);
            } else {
                printf("JEQ: Condition false. Continue normally.\n");
            }
            break;
        case 8: // XORI
            result = registers[instr->r1] ^ instr->imm;
            break;
        case 9: // MOVR (address calculation only)
            // Actual load in MEM, write in WB
            break;
        case 10: // MOVM (store)
            // Actual store in MEM
            break;
        case 11: // JMP
            flush_flag = 1;
            printf("JMP: Decoded addr = %u\n", instr->addr);
            branch_target = instr->addr;
            printf("JMP: Jumping to address %u\n", branch_target);
            break;
        default:
            fprintf(stderr, "Invalid opcode: %d in Execute\n", instr->opcode);
            break;
    }
    printf("Execute Stage: Opcode %d, Result = %d\n", instr->opcode, result);
    return (uint32_t)result;
}


// Memory Access
void memory_access(uint32_t *memory, Instruction *instr) {
    if (instr->opcode == 10) { // MOVM (store)
        // Store value from r1 into memory at address (registers[r2] + imm)
        uint32_t address = registers[instr->r2] + instr->imm;
        if (address >= MEMORY_SIZE) {
            fprintf(stderr, "MOVM Error: Address %u out of bounds.\n", address);
            exit(EXIT_FAILURE);
        }
        memory[address] = registers[instr->r1];
        printf("[MEM] MOVM: Stored value %d from R%d into memory[%u]\n", (int32_t)registers[instr->r1], instr->r1, address);
    } else if (instr->opcode == 9) { // MOVR (load)
        // Load value from memory at address (registers[r2] + imm) into finalResult
        uint32_t address = registers[instr->r2] + instr->imm;
        if (address >= MEMORY_SIZE) {
            fprintf(stderr, "MOVR Error: Address %u out of bounds.\n", address);
            exit(EXIT_FAILURE);
        }
        finalResult = memory[address];
        printf("[MEM] MOVR: Read value %d from memory[%u]\n", (int32_t)finalResult, address);
    }
}


// Write Back
void write_back(Instruction *instr, uint32_t result) {
    // Write for all R-type (0-5), MOVI (6), XORI (8), MOVR (9)
    if (instr->opcode >= 0 && instr->opcode <= 5) {
        printf("[WB] R-type: Writing %d to R%d\n", (int32_t)result, instr->r1);
        safe_register_write(instr->r1, result & 0xFFFFFFFF, "Write Back Stage");
    } else if (instr->opcode == 6) {
        printf("[WB] MOVI: Writing %d to R%d\n", (int32_t)result, instr->r1);
        safe_register_write(instr->r1, result & 0xFFFFFFFF, "Write Back Stage (MOVI)");
    } else if (instr->opcode == 8) {
        printf("[WB] XORI: Writing %d to R%d\n", (int32_t)result, instr->r1);
        safe_register_write(instr->r1, result & 0xFFFFFFFF, "Write Back Stage (XORI)");
    } else if (instr->opcode == 9) { // MOVR writes from finalResult
        printf("[WB] MOVR: Writing %d to R%d\n", (int32_t)finalResult, instr->r1);
        safe_register_write(instr->r1, finalResult & 0xFFFFFFFF, "Write Back Stage (MOVR)");
    }
    // No write-back for MOVM (10), JEQ (7), JMP (11)
}





















