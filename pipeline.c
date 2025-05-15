#include "pipeline.h"
#include "memory.h" // Include the memory.h header
#include <stdio.h>
#include <stdlib.h> // Include for exit()
#include <stdint.h>


int cycle = 1;
void instruction_fetch(uint32_t instruction_memory[], uint32_t *pc, uint32_t *if_id_register, int cycle);
void instruction_decode(uint32_t if_id_register, uint32_t *id_ex_register, int cycle);
void execute(uint32_t id_ex_register, uint32_t *ex_mem_register, int cycle);
void memory_access(uint32_t ex_mem_register, uint32_t *mem_wb_register, int cycle);
void write_back(uint32_t mem_wb_register, int cycle);


struct instruction_R{
    uint32_t opcode:4;
    uint32_t r1:5;
    uint32_t r2:5;
    uint32_t r3:5;
    uint32_t shamt:13;
};

struct instruction_I{
    uint32_t opcode:4;
    uint32_t r1:5;
    uint32_t r2:5;
    uint32_t imm:18;
};

struct instruction_J{
    uint32_t opcode:4;
    uint32_t address:28;
};

void run_pipeline(uint32_t instruction_memory[], int num_instructions) {
    uint32_t pc = 0; // Program Counter
    uint32_t if_id_register = 0;
    uint32_t id_ex_register = 0;
    uint32_t ex_mem_register = 0;
    uint32_t mem_wb_register = 0;
      // Clock cycle counter


    // Loop through the instructions, simulating the pipeline stages
    while (pc < num_instructions || if_id_register != 0 || id_ex_register != 0 || ex_mem_register != 0 || mem_wb_register != 0) {
        // Simulate the pipeline stages.  Handle the fact that IF and MEM
        // cannot occur in the same cycle.
        if (cycle % 2 != 0) {
            instruction_fetch(instruction_memory, &pc, &if_id_register, cycle);
        }
        else{
            memory_access(ex_mem_register, &mem_wb_register, cycle);
        }
        instruction_decode(if_id_register, &id_ex_register, cycle);
        execute(id_ex_register, &ex_mem_register, cycle);
        write_back(mem_wb_register, cycle);

        printf("Cycle %d:\n", cycle);
        printf("  PC: %u, IF_ID: 0x%08X, ID_EX: 0x%08X, EX_MEM: 0x%08X, MEM_WB: 0x%08X\n", pc, if_id_register, id_ex_register, ex_mem_register, mem_wb_register);

        cycle++;
    }
}


// IF (odd cycles only, and not flushing)
if (cycle % 2 == 1 && instructions_fetched < num_instructions && pipeline[0].valid == 0 && !flush_pipeline) {
    pipeline[0].valid = 1;
    pipeline[0].instr = read_memory(PC); // Use memory.c's read_memory
    pipeline[0].pc_at_fetch = PC;
    pipeline[0].id_cycles = 2;
    PC++;
    instructions_fetched++;
}
// Implement the pipeline stage functions
 uint32_t instruction_fetch(uint32_t instruction_memory[], uint32_t *pc, uint32_t *if_id_register, int cycle) {
    if (*pc < sizeof(instruction_memory) / sizeof(instruction_memory[0]))
    {
     print("the address is %d not accessible", *pc);
     return -1;
    }
    else
        printf("instruction %d fetched: 0x%08X\n", *pc, instruction_memory[*pc]);
       return instruction_memory[(*pc)++];
}

void instruction_decode(uint32_t if_id_register, uint32_t *id_ex_register, int cycle) {
    uint32_t opcode = (if_id_register >> 28) & 0xF; // Extract opcode (bits 31-28)

    // Decode based on the opcode
    if (opcode <= 5) { // R-type instruction
        struct instruction_R decoded_R;
        decoded_R.opcode = opcode;
        decoded_R.r1 = (if_id_register >> 23) & 0x1F; // Extract r1 (bits 27-23)
        decoded_R.r2 = (if_id_register >> 18) & 0x1F; // Extract r2 (bits 22-18)
        decoded_R.r3 = (if_id_register >> 13) & 0x1F; // Extract r3 (bits 17-13)
        decoded_R.shamt = if_id_register & 0x1FFF;    // Extract shift amount (bits 12-0)

        printf("Decoded R-type: opcode=%u, r1=%u, r2=%u, r3=%u, shamt=%u\n",
               decoded_R.opcode, decoded_R.r1, decoded_R.r2, decoded_R.r3, decoded_R.shamt);
    } else if (opcode >= 6 && opcode <= 10) { // I-type instruction
        struct instruction_I decoded_I;
        decoded_I.opcode = opcode;
        decoded_I.r1 = (if_id_register >> 23) & 0x1F; // Extract r1 (bits 27-23)
        decoded_I.r2 = (if_id_register >> 18) & 0x1F; // Extract r2 (bits 22-18)
        decoded_I.imm = if_id_register & 0x3FFFF;     // Extract immediate (bits 17-0)

        printf("Decoded I-type: opcode=%u, r1=%u, r2=%u, imm=%u\n",
               decoded_I.opcode, decoded_I.r1, decoded_I.r2, decoded_I.imm);
    } else if (opcode == 11) { // J-type instruction
        struct instruction_J decoded_J;
        decoded_J.opcode = opcode;
        decoded_J.address = if_id_register & 0xFFFFFFF; // Extract address (bits 27-0)

        printf("Decoded J-type: opcode=%u, address=%u\n",
               decoded_J.opcode, decoded_J.address);
    } else {
        fprintf(stderr, "Invalid opcode: %u\n", opcode);
        exit(EXIT_FAILURE);
    }

    // Pass the instruction to the next pipeline stage
    *id_ex_register = if_id_register;
}

void execute(uint32_t id_ex_register, uint32_t *ex_mem_register, int cycle) {


    switch (opcode) {
        case 0: // ADD
            registers[r1] = registers[r2] + registers[r3];
            break;
        case 1: // SUB
            registers[r1] = registers[r2] - registers[r3];
            break;
        case 2: // AND
            registers[r1] = registers[r2] & registers[r3];
            break;
        case 3: // OR
            registers[r1] = registers[r2] | registers[r3];
            break;
        case 4: // LSL
            registers[r1] = registers[r2] << shamt;
            break;
        case 5: // LSR
            registers[r1] = registers[r2] >> shamt;
            break;
        case 6: // MOVI
            registers[r1] = imm;
            break;
        case 7: // JEQ
            if (registers[r1] == registers[r2]) {
                extern uint32_t pc;
                pc = pc + imm - 1; // Adjust PC for jump
            }
            break;
        case 8: // JMP
            extern uint32_t pc;
            pc = imm; // Set PC to the jump address
            break;
        case 9: // LW
            registers[r1] = read_memory(registers[r2] + imm);
            break;
        case 10: // SW
            write_memory(registers[r2] + imm, registers[r1]);
            break;
        case 11: // NOP
            // No operation
            break;
        default:
            fprintf(stderr, "Invalid opcode: %u\n", opcode);
            exit(EXIT_FAILURE);
    }

    *ex_mem_register = id_ex_register;
}

void memory_access(uint32_t ex_mem_register, uint32_t *mem_wb_register, int cycle) {
 
}

void write_back(uint32_t mem_wb_register, int cycle) {
    uint32_t opcode = (mem_wb_register >> 28) & 0xF;
    uint32_t r1 = (mem_wb_register >> 23) & 0x1F;

    // Write the result to the register file
    //  Make sure it is not NOP instruction
    if (opcode != 11 && opcode != 7 && opcode != 8 && opcode != 10)
    {
       // write_register(r1, result);  // Use the write_register function from memory.c
    }
}
