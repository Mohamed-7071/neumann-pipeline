#include "pipeline.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "main.h"

int cycle = 1;
uint32_t registers[32]; // Simple register file

// Structs for instruction decoding
struct instruction_R {
    uint32_t opcode : 4;
    uint32_t r1 : 5;
    uint32_t r2 : 5;
    uint32_t r3 : 5;
    uint32_t shamt : 13;
};

struct instruction_I {
    uint32_t opcode : 4;
    uint32_t r1 : 5;
    uint32_t r2 : 5;
    uint32_t imm : 18;
};

struct instruction_J {
    uint32_t opcode : 4;
    uint32_t address : 28;
};

// Function prototypes
uint32_t instruction_fetch(uint32_t instruction_memory[], uint32_t *pc, int num_instructions);
void instruction_decode(uint32_t if_id_register, uint32_t *id_ex_register);
void execute(uint32_t id_ex_register, uint32_t *ex_mem_register, uint32_t *pc);
void memory_access(uint32_t ex_mem_register, uint32_t *mem_wb_register);
void write_back(uint32_t mem_wb_register);

void run_pipeline(uint32_t instruction_memory[], int num_instructions) {
    uint32_t pc = 0;
    uint32_t if_id_register = 0;
    uint32_t id_ex_register = 0;
    uint32_t ex_mem_register = 0;
    uint32_t mem_wb_register = 0;

    while (pc < num_instructions || if_id_register || id_ex_register || ex_mem_register || mem_wb_register) {
        if (cycle % 2 != 0) {
            if_id_register = instruction_fetch(instruction_memory, &pc, num_instructions);
        } else {
            memory_access(ex_mem_register, &mem_wb_register);
        }

        instruction_decode(if_id_register, &id_ex_register);
        execute(id_ex_register, &ex_mem_register, &pc);
        write_back(mem_wb_register);

        printf("Cycle %d:\n", cycle);
        printf("  PC: %u, IF_ID: 0x%08X, ID_EX: 0x%08X, EX_MEM: 0x%08X, MEM_WB: 0x%08X\n",
               pc, if_id_register, id_ex_register, ex_mem_register, mem_wb_register);

        cycle++;
    }
}

uint32_t instruction_fetch(uint32_t instruction_memory[], uint32_t *pc, int num_instructions) {
    if (*pc >= num_instructions) {
        printf("PC out of bounds: %u\n", *pc);
        return 0;
    }

    uint32_t instr = instruction_memory[*pc];
    printf("Fetched instruction %d: 0x%08X\n", *pc, instr);
    (*pc)++;
    return instr;
}

void instruction_decode(uint32_t if_id_register, uint32_t *id_ex_register) {
    uint32_t opcode = (if_id_register >> 28) & 0xF;

    if (opcode <= 5) { // R-type
        struct instruction_R r = {
            .opcode = opcode,
            .r1 = (if_id_register >> 23) & 0x1F,
            .r2 = (if_id_register >> 18) & 0x1F,
            .r3 = (if_id_register >> 13) & 0x1F,
            .shamt = if_id_register & 0x1FFF
        };
        printf("Decoded R-type: opcode=%u, r1=%u, r2=%u, r3=%u, shamt=%u\n",
               r.opcode, r.r1, r.r2, r.r3, r.shamt);
    } else if (opcode >= 6 && opcode <= 10) { // I-type
        struct instruction_I i = {
            .opcode = opcode,
            .r1 = (if_id_register >> 23) & 0x1F,
            .r2 = (if_id_register >> 18) & 0x1F,
            .imm = if_id_register & 0x3FFFF
        };
        printf("Decoded I-type: opcode=%u, r1=%u, r2=%u, imm=%u\n",
               i.opcode, i.r1, i.r2, i.imm);
    } else if (opcode == 11) { // J-type
        struct instruction_J j = {
            .opcode = opcode,
            .address = if_id_register & 0xFFFFFFF
        };
        printf("Decoded J-type: opcode=%u, address=%u\n",
               j.opcode, j.address);
    } else {
        fprintf(stderr, "Invalid opcode: %u\n", opcode);
        exit(EXIT_FAILURE);
    }

    *id_ex_register = if_id_register; // Pass to next stage
}

void execute(uint32_t id_ex_register, uint32_t *ex_mem_register, uint32_t *pc) {
    uint32_t opcode = (id_ex_register >> 28) & 0xF;
    uint32_t r1 = (id_ex_register >> 23) & 0x1F;
    uint32_t r2 = (id_ex_register >> 18) & 0x1F;
    uint32_t r3 = (id_ex_register >> 13) & 0x1F;
    uint32_t shamt = id_ex_register & 0x1FFF;
    uint32_t imm = id_ex_register & 0x3FFFF;

    switch (opcode) {
        case 0: registers[r1] = registers[r2] + registers[r3]; break;
        case 1: registers[r1] = registers[r2] - registers[r3]; break;
        case 2: registers[r1] = registers[r2] & registers[r3]; break;
        case 3: registers[r1] = registers[r2] | registers[r3]; break;
        case 4: registers[r1] = registers[r2] << shamt; break;
        case 5: registers[r1] = registers[r2] >> shamt; break;
        case 6: registers[r1] = imm; break;
        case 7: if (registers[r1] == registers[r2]) *pc += imm - 1; break;
        case 8: *pc = imm; break;
        case 9: registers[r1] = read_memory(registers[r2] + imm); break;
        case 10: write_memory(registers[r2] + imm, registers[r1]); break;
        case 11: break; // NOP
        default:
            fprintf(stderr, "Invalid opcode in execute: %u\n", opcode);
            exit(EXIT_FAILURE);
    }

    *ex_mem_register = id_ex_register;
}

void memory_access(uint32_t ex_mem_register, uint32_t *mem_wb_register) {
    // Placeholder: In actual design, you'd extract and process LW/SW again here.
    *mem_wb_register = ex_mem_register;
}

void write_back(uint32_t mem_wb_register) {
    uint32_t opcode = (mem_wb_register >> 28) & 0xF;
    uint32_t r1 = (mem_wb_register >> 23) & 0x1F;

    if (opcode != 11 && opcode != 7 && opcode != 8 && opcode != 10) {
        // write_register(r1, result); // Replace with actual logic if needed
        printf("Write back: Register %u = %u\n", r1, registers[r1]);
    }
}
