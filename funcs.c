// funcs.c
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "register.c"
#include "memory.c"
# include "funcs.h"
extern bool flagwork = true;

uint32_t instruction = 0;
int flush_flag = 0;
uint32_t branch_target = 0;

// Globals for MOVR flow
uint32_t movr_address = 0;
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
    PC++;  // Global PC incremented here
    return instr;


}


// Instruction Decode
void instruction_decode(uint32_t binary_instruction, Instruction *instr) {
    uint8_t opcodee = (binary_instruction >> 28) & 0xF;
    instr->opcode = opcodee;

    if (opcodee >= 0 && opcodee <= 5) {
        // R-Type
        instr->r1 = (binary_instruction >> 23) & 0x1F;
        instr->r2 = (binary_instruction >> 18) & 0x1F;
        instr->r3 = (binary_instruction >> 13) & 0x1F;
        instr->shamt = binary_instruction & 0x1FFF;

        instr->imm = 0;
        instr->addr = 0;
    } else if (opcodee >= 6 && opcodee <= 10) {
        // I-Type
        instr->r1 = (binary_instruction >> 23) & 0x1F;
        instr->r2 = (binary_instruction >> 18) & 0x1F;
        instr->imm = binary_instruction & 0x3FFFF;

        // Sign-extend immediate
        if (instr->imm & (1 << 17)) {
            instr->imm |= ~0x3FFFF;
        }

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
uint32_t execute(Instruction *instr) {
    int result = 0;

    switch (instr->opcode) {
        // R-TYPE Instructions
        case 0: result = registers[instr->r2] + registers[instr->r3]; break;
        case 1: result = registers[instr->r2] - registers[instr->r3]; break;
        case 2: result = registers[instr->r2] * registers[instr->r3]; break;
        case 3: result = registers[instr->r2] & registers[instr->r3]; break;
        case 4: result = registers[instr->r2] << (instr->shamt & 0x1FFF); break;
        case 5: result = (uint32_t)registers[instr->r2] >> (instr->shamt & 0x1FFF); break;

        // I-TYPE Instructions
        case 6: result = instr->imm; break;
        case 7:
            if (registers[instr->r1] == registers[instr->r2]) {
                flush_flag = 1;
                branch_target = PC + instr->imm;
                printf("JEQ: Branch taken. New PC will be %u\n", branch_target);
            } else {
                printf("JEQ: Condition false. Continue normally.\n");
            }
            break;
        case 8: result = registers[instr->r1] ^ instr->imm; break;

        case 9: // MOVR — calculate address only
            movr_address = registers[instr->r2] + instr->imm;
            printf("Execute Stage: Calculated MOVR address = %u\n", movr_address);
            break;

        case 10: // MOVM — handled in MEM
            break;

        // J-TYPE Instruction
        case 11:
            flush_flag = 1;
            branch_target = instr->addr;
            printf("JMP: Jumping to address %u\n", branch_target);
            break;

        default:
            fprintf(stderr, "Invalid opcode: %d in Execute\n", instr->opcode);
            break;
    }

    printf("Execute Stage: Opcode %d, Result = %d\n", instr->opcode, result);
    return result;
}


// Memory Access
void memory_access(uint32_t *memory, Instruction *instr) {
    if (instr->opcode == 10) { // MOVM (store)
        uint32_t address = registers[instr->r2] + instr->imm;
        if (address >= MEMORY_SIZE) {
            fprintf(stderr, "MOVM Error: Address %u out of bounds.\n", address);
            exit(EXIT_FAILURE);
        }
        memory[address] = registers[instr->r1];
        printf("MOVM: Stored value %u from R%d into memory[%u]\n",
               registers[instr->r1], instr->r1, address);

    } else if (instr->opcode == 9) { // MOVR — now reads memory here
        if (movr_address >= MEMORY_SIZE) {
            fprintf(stderr, "MOVR Error: Address %u out of bounds.\n", movr_address);
            exit(EXIT_FAILURE);
        }
        finalResult = memory[movr_address];
        printf("MOVR: Read value %u from memory[%u]\n", finalResult, movr_address);
    }
}


// Write Back
void write_back(Instruction *instr, uint32_t result) {
    if (instr->opcode == 9) { // MOVR writes from finalResult
        safe_register_write(instr->r1, finalResult, "Write Back Stage (MOVR)");
    }
    else if (instr->opcode <= 6 || instr->opcode == 8) {
        safe_register_write(instr->r1, result, "Write Back Stage");
    }
}
