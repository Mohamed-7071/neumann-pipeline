#include "pipeline.h"
#include "memory.h" // Include the memory.h header
#include <stdio.h>
#include <stdlib.h> // Include for exit()

// Pipeline stage functions
void instruction_fetch(uint32_t instruction_memory[], uint32_t *pc, uint32_t *if_id_register, int cycle);
void instruction_decode(uint32_t if_id_register, uint32_t *id_ex_register, int cycle);
void execute(uint32_t id_ex_register, uint32_t *ex_mem_register, int cycle);
void memory_access(uint32_t ex_mem_register, uint32_t *mem_wb_register, int cycle);
void write_back(uint32_t mem_wb_register, int cycle);

// Global array to hold the register file.  This is defined in memory.c, but we
// declare it here so pipeline.c can access it.
extern uint32_t registers[32];

void run_pipeline(uint32_t instruction_memory[], int num_instructions) {
    uint32_t pc = 0; // Program Counter
    uint32_t if_id_register = 0;
    uint32_t id_ex_register = 0;
    uint32_t ex_mem_register = 0;
    uint32_t mem_wb_register = 0;
    int cycle = 1;    // Clock cycle counter

    // Initialize pipeline registers (if needed)

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



// Implement the pipeline stage functions
void instruction_fetch(uint32_t instruction_memory[], uint32_t *pc, uint32_t *if_id_register, int cycle) {
    if (*pc < sizeof(instruction_memory) / sizeof(instruction_memory[0]))
    {
        // Fetch instruction from memory using PC
        // Increment PC
        *if_id_register = instruction_memory[*pc];
        (*pc)++;
    }
    else
    {
        *if_id_register = 0;
    }
}

void instruction_decode(uint32_t if_id_register, uint32_t *id_ex_register, int cycle) {
    // Decode the instruction (extract opcode, register numbers, immediate values)
    // Read register values from the register file
    *id_ex_register = if_id_register; // Pass the instruction to the next stage.
}

void execute(uint32_t id_ex_register, uint32_t *ex_mem_register, int cycle) {
    // Perform ALU operations based on the instruction.
    // Use a switch statement to handle different opcodes.
    uint32_t opcode = (id_ex_register >> 28) & 0xF; // Extract opcode (bits 31-28)
    uint32_t r1 = (id_ex_register >> 23) & 0x1F;     // Extract r1 (bits 27-23)
    uint32_t r2 = (id_ex_register >> 18) & 0x1F;     // Extract r2 (bits 22-18)
    uint32_t r3 = (id_ex_register >> 13) & 0x1F;     // Extract r3 (bits 17-13)
    uint32_t imm = id_ex_register & 0x3FFFF;          // Extract immediate (bits 17-0)
    uint32_t shamt = id_ex_register & 0x1FFF;        // Extract shift amount

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
                //  Jump by the immediate value.  Since PC is incremented in IF stage,
                //  we subtract 1 here.
                //  Also, the immediate is added to the *current* PC, which has
                //  already been incremented.  So we subtract 1 to get the correct behavior.
                 extern uint32_t pc;
                 pc = pc + imm - 1;
            }
            break;
        case 8: // JMP
             extern uint32_t pc;
             pc = imm;
             break;
        case 9: // LW
             registers[r1] = read_memory(registers[r2] + imm);
             break;
        case 10: // SW
             write_memory(registers[r2] + imm, registers[r1]);
             break;
        case 11: // NOP
             break;
        default:
            fprintf(stderr, "Invalid opcode: %u\n", opcode);
            exit(EXIT_FAILURE);
    }
    *ex_mem_register = id_ex_register;
}

void memory_access(uint32_t ex_mem_register, uint32_t *mem_wb_register, int cycle) {
    // Perform memory read or write if required by the instruction
    //  Use the read_memory() and write_memory() functions from memory.c
    *mem_wb_register = ex_mem_register;
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
```