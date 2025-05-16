#include "pipeline.h"
#include "memory.h" // Include the memory.h header
#include "registers.h" // Include the registers.h header
#include <stdio.h>
#include <stdlib.h> // Include for exit()
#include <stdint.h>

#define MAX_PIPELINE_DEPTH 5
#define MAX_INSTRUCTIONS 100

typedef enum { STAGE_NONE, STAGE_IF, STAGE_ID, STAGE_EX, STAGE_MEM, STAGE_WB, STAGE_DONE } Stage;

typedef struct {
    Instruction inst;
    Stage stage;
    int stage_cycles_left;
    int active; // 1 if this slot is occupied by an instruction
} PipelineSlot;

int cycle = 1;
uint32_t instruction_fetch(uint32_t instruction_memory[], uint32_t *pc, uint32_t *if_id_register, int cycle);
void instruction_decode(uint32_t if_id_register, Instruction *id_ex, int cycle);
void execute(Instruction *id_ex, uint32_t *ex_mem_register, int cycle);
void memory_access(uint32_t ex_mem_register, uint32_t *mem_wb_register, int cycle);
void write_back(uint32_t mem_wb_register, int cycle);


// Instruction Structure
typedef struct {
    uint32_t binary; // 32-bit instruction
    int opcode;
    int r1, r2, r3, shamt; // R-format fields
    int imm; // I-format immediate
    int addr; // J-format address
    int pc; // PC at fetch time (for branches)
    int valid; // Flag to indicate valid instruction
} Instruction;

// Pipeline Stage Structure
typedef struct {
    Instruction inst;
    int stage_cycle; // Tracks cycles spent in stage (for ID, EX)
} PipelineStage;


void run_pipeline(uint32_t instruction_memory[], int num_instructions) {
    PipelineSlot pipeline[MAX_INSTRUCTIONS] = {0};
    int instructions_in_pipeline = 0;
    int next_fetch = 0;
    int completed = 0;
    int cycle = 1;

    while (completed < num_instructions) {
        // 1. Advance WB stage (finish instructions)
        for (int i = 0; i < num_instructions; ++i) {
            if (pipeline[i].active && pipeline[i].stage == STAGE_WB && pipeline[i].stage_cycles_left == 0) {
                write_back(pipeline[i].inst.binary, cycle);
                pipeline[i].stage = STAGE_DONE;
                pipeline[i].active = 0;
                completed++;
            }
        }

        // 2. Advance MEM stage
        int mem_busy = 0;
        for (int i = 0; i < num_instructions; ++i) {
            if (pipeline[i].active && pipeline[i].stage == STAGE_MEM) {
                if (pipeline[i].stage_cycles_left == 0) {
                    mem_busy = 1;
                    memory_access(pipeline[i].inst.binary, &pipeline[i].inst.binary, cycle);
                    pipeline[i].stage = STAGE_WB;
                    pipeline[i].stage_cycles_left = 1;
                } else {
                    pipeline[i].stage_cycles_left--;
                }
            }
        }

        // 3. Advance EX stage
        for (int i = 0; i < num_instructions; ++i) {
            if (pipeline[i].active && pipeline[i].stage == STAGE_EX) {
                if (pipeline[i].stage_cycles_left == 0) {
                    execute(&pipeline[i].inst, &pipeline[i].inst.binary, cycle);
                    pipeline[i].stage = STAGE_MEM;
                    pipeline[i].stage_cycles_left = 1;
                } else {
                    pipeline[i].stage_cycles_left--;
                }
            }
        }

        // 4. Advance ID stage
        for (int i = 0; i < num_instructions; ++i) {
            if (pipeline[i].active && pipeline[i].stage == STAGE_ID) {
                if (pipeline[i].stage_cycles_left == 0) {
                    instruction_decode(pipeline[i].inst.binary, &pipeline[i].inst, cycle);
                    pipeline[i].stage = STAGE_EX;
                    pipeline[i].stage_cycles_left = 2;
                } else {
                    pipeline[i].stage_cycles_left--;
                }
            }
        }

        // 5. Fetch new instruction if allowed (every 2 cycles, and only if MEM is not running)
        if ((cycle % 2 == 1) && !mem_busy && next_fetch < num_instructions) {
            pipeline[next_fetch].inst.binary = instruction_fetch(instruction_memory, &next_fetch, &pipeline[next_fetch].inst.binary, cycle);
            pipeline[next_fetch].stage = STAGE_ID;
            pipeline[next_fetch].stage_cycles_left = 2;
            pipeline[next_fetch].active = 1;
            instructions_in_pipeline++;
        }

        // Print pipeline state for debugging
        printf("Cycle %d:\n", cycle);
        for (int i = 0; i < num_instructions; ++i) {
            if (pipeline[i].active) {
                printf("  Instruction %d in stage %d (cycles left: %d)\n", i+1, pipeline[i].stage, pipeline[i].stage_cycles_left);
            }
        }
        cycle++;
    }
}

uint32_t instruction_fetch(uint32_t instruction_memory[], uint32_t *pc, uint32_t *if_id_register, int cycle) {
    if (*pc < sizeof(instruction_memory) / sizeof(instruction_memory[0])) {
        print("the address is %d not accessible", *pc);
        return -1;
    } else {
        printf("instruction %d fetched: 0x%08X\n", *pc, instruction_memory[*pc]);
    }
    cycle++;
    return instruction_memory[(*pc)++];
}

void instruction_decode(uint32_t if_id_register, Instruction *id_ex, int cycle) {
    id_ex->binary = if_id_register;
    id_ex->opcode = (if_id_register >> 28) & 0xF;
    id_ex->pc = 0; // Set if you track PC elsewhere
    id_ex->valid = 1;

    if (id_ex->opcode <= 5) { // R-type
        id_ex->r1 = (if_id_register >> 23) & 0x1F;
        id_ex->r2 = (if_id_register >> 18) & 0x1F;
        id_ex->r3 = (if_id_register >> 13) & 0x1F;
        id_ex->shamt = if_id_register & 0x1FFF;
        id_ex->imm = 0;
        id_ex->addr = 0;
        printf("Decoded R-type: opcode=%u, r1=%u, r2=%u, r3=%u, shamt=%u\n",
               id_ex->opcode, id_ex->r1, id_ex->r2, id_ex->r3, id_ex->shamt);
    } else if (id_ex->opcode >= 6 && id_ex->opcode <= 10) { // I-type
        id_ex->r1 = (if_id_register >> 23) & 0x1F;
        id_ex->r2 = (if_id_register >> 18) & 0x1F;
        id_ex->imm = if_id_register & 0x3FFFF;
        id_ex->r3 = 0;
        id_ex->shamt = 0;
        id_ex->addr = 0;
        printf("Decoded I-type: opcode=%u, r1=%u, r2=%u, imm=%u\n",
               id_ex->opcode, id_ex->r1, id_ex->r2, id_ex->imm);
    } else if (id_ex->opcode == 11) { // J-type
        id_ex->addr = if_id_register & 0xFFFFFFF;
        id_ex->r1 = id_ex->r2 = id_ex->r3 = id_ex->shamt = id_ex->imm = 0;
        printf("Decoded J-type: opcode=%u, address=%u\n",
               id_ex->opcode, id_ex->addr);
    } else {
        id_ex->valid = 0;
        fprintf(stderr, "Invalid opcode: %u\n", id_ex->opcode);
        exit(EXIT_FAILURE);
    }
    cycle++;
}

void execute(Instruction *id_ex, uint32_t *ex_mem_register, int cycle) {
    switch (id_ex->opcode) {
        case 0: // ADD
            registers[id_ex->r1] = registers[id_ex->r2] + registers[id_ex->r3];
            break;
        case 1: // SUB
            registers[id_ex->r1] = registers[id_ex->r2] - registers[id_ex->r3];
            break;
        case 2: // AND
            registers[id_ex->r1] = registers[id_ex->r2] & registers[id_ex->r3];
            break;
        case 3: // OR
            registers[id_ex->r1] = registers[id_ex->r2] | registers[id_ex->r3];
            break;
        case 4: // LSL
            registers[id_ex->r1] = registers[id_ex->r2] << id_ex->shamt;
            break;
        case 5: // LSR
            registers[id_ex->r1] = registers[id_ex->r2] >> id_ex->shamt;
            break;
        case 6: // MOVI
            registers[id_ex->r1] = id_ex->imm;
            break;
        case 7: // JEQ
            if (registers[id_ex->r1] == registers[id_ex->r2]) {
                extern uint32_t pc;
                pc = pc + id_ex->imm - 1;
            }
            break;
        case 8: // JMP
            extern uint32_t pc;
            pc = id_ex->addr;
            break;
        case 9: // LW
            registers[id_ex->r1] = read_memory(registers[id_ex->r2] + id_ex->imm);
            break;
        case 10: // SW
            write_memory(registers[id_ex->r2] + id_ex->imm, registers[id_ex->r1]);
            break;
        case 11: // NOP
            // No operation
            break;
        default:
            fprintf(stderr, "Invalid opcode: %u\n", id_ex->opcode);
            exit(EXIT_FAILURE);
    }
    *ex_mem_register = id_ex->binary; // or pass the struct if you want
    cycle++;
}

void memory_access(uint32_t ex_mem_register, uint32_t *mem_wb_register, int cycle) {
    uint32_t opcode = (ex_mem_register >> 28) & 0xF; // Extract opcode
    uint32_t r1, r2, imm, address;

    printf("Cycle %d: Memory Access Stage\n", cycle);

    switch (opcode) {
        case 9: // MOVR R1 R2 IMM
            r1 = (ex_mem_register >> 23) & 0x1F;
            r2 = (ex_mem_register >> 18) & 0x1F;
            imm = ex_mem_register & 0x3FFFF;
            uint32_t mem_addr_r = get_register(r2) + imm; // Calculate memory address
            if(mem_addr_r >= MEMORY_SIZE){
                fprintf(stderr, "Memory Access Violation: MOVR, address 0x%08X\n", mem_addr_r);
                exit(EXIT_FAILURE);
            }
            uint32_t read_data = read_memory(mem_addr_r); // Read from memory
            printf("  MOVR R%u, R%u, %d: Read 0x%08X from memory address 0x%08X\n", r1, r2, imm, read_data, mem_addr_r);
            // Pass data to the next stage.  Critical to pass the *data* read, not the address.
            *mem_wb_register = (opcode << 28) | (r1 << 23) | (read_data & 0x7FFFFFF); //store read data in the lower 27 bits
            break;

        case 10: // MOVM R1 R2 IMM
            r1 = (ex_mem_register >> 23) & 0x1F;
            r2 = (ex_mem_register >> 18) & 0x1F;
            imm = ex_mem_register & 0x3FFFF;
            uint32_t mem_addr_w = get_register(r2) + imm; // Calculate memory address
             if(mem_addr_w >= MEMORY_SIZE){
                fprintf(stderr, "Memory Access Violation: MOVM, address 0x%08X\n", mem_addr_w);
                exit(EXIT_FAILURE);
            }
            uint32_t write_data = get_register(r1);
            write_memory(mem_addr_w, write_data);         // Write to memory
            printf("  MOVM R%u, R%u, %d: Wrote 0x%08X to memory address 0x%08X\n", r1, r2, imm, write_data, mem_addr_w);
            // Pass the original instruction (with the value of R1) to the next stage for WB.
            *mem_wb_register = ex_mem_register;
            break;

        default:
            // For other instructions, no memory access is performed in this stage.
            // Just pass the data from the previous stage to the next.
            *mem_wb_register = ex_mem_register;
            printf("  No memory access for this instruction.\n");
            break;
    }
    cycle++;
}

void write_back(uint32_t mem_wb_register, int cycle) {
    uint32_t opcode = (mem_wb_register >> 28) & 0xF;
    uint32_t r1, result;

    printf("Cycle %d: Write Back Stage\n", cycle);

    switch (opcode) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
            r1 = (mem_wb_register >> 23) & 0x1F;
            result = mem_wb_register & 0x7FFFFFF;
            set_register(r1, result);
            printf("  Write Back: R%u = 0x%08X\n", r1, result);
            break;

        case 10:
            printf("  Write Back: No write back for MOVM.\n");
            break;

        case 7:
        case 11:
            printf("  Write Back: No write back for JEQ or JMP.\n");
            break;
        default:
            printf("  Write Back: No write back for this instruction.\n");
            break;
    }
    cycle++;
}
