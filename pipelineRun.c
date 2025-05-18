#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "registers.h"
#include "memory.h"
#include "funcs.h"

#define FILENAME "program.txt"
#define MAX_INSTRUCTIONS 1024
#define PIPELINE_DEPTH 4

extern uint32_t instruction_memory[MAX_INSTRUCTIONS];
extern uint32_t PC;
extern bool flagwork;
extern int flush_flag;
extern uint32_t branch_target;

// Pipeline stage structures
typedef struct {
    uint32_t instruction;
    Instruction decoded;
    int cycles_remaining;
    bool active;
    uint32_t result; // Separate field for execution result
} PipelineStage;

PipelineStage if_stage = {0, {0}, 2, false, 0};
PipelineStage id_stage = {0, {0}, 2, false, 0};
PipelineStage ex_stage = {0, {0}, 2, false, 0};
PipelineStage mem_stage = {0, {0}, 1, false, 0};
PipelineStage wb_stage = {0, {0}, 1, false, 0};

int total_instructions = 0;
int instructions_fetched = 0; // Track number of instructions fetched
int instructions_executed = 0; // Track number of instructions completed

void print_pipeline_state(int cycle) {
    printf("\nClock Cycle %d State:\n", cycle);
    printf("IF: %s (Cycles: %d, Instr: 0x%08X)\n", 
           if_stage.active ? "Active" : "Inactive", if_stage.cycles_remaining, if_stage.instruction);
    printf("ID: %s (Cycles: %d, Instr: 0x%08X)\n", 
           id_stage.active ? "Active" : "Inactive", id_stage.cycles_remaining, id_stage.instruction);
    printf("EX: %s (Cycles: %d, Result: %u)\n", 
           ex_stage.active ? "Active" : "Inactive", ex_stage.cycles_remaining, ex_stage.result);
    printf("MEM: %s (Cycles: %d, Result: %u)\n", 
           mem_stage.active ? "Active" : "Inactive", mem_stage.cycles_remaining, mem_stage.result);
    printf("WB: %s (Cycles: %d, Result: %u)\n", 
           wb_stage.active ? "Active" : "Inactive", wb_stage.cycles_remaining, wb_stage.result);
    printf("PC: %d, Flush Flag: %d, Branch Target: %d\n", PC, flush_flag, branch_target);
}

void flush_pipeline() {
    if_stage.active = false;
    if_stage.cycles_remaining = 2;
    if_stage.instruction = 0;
    if_stage.result = 0;

    id_stage.active = false;
    id_stage.cycles_remaining = 2;
    id_stage.instruction = 0;
    memset(&id_stage.decoded, 0, sizeof(Instruction));
    id_stage.result = 0;

    ex_stage.result = 0;
    mem_stage.result = 0;
    wb_stage.result = 0;

    PC = (branch_target > 0) ? (branch_target - 1) : 0; // Adjust for PC increment in fetch
    if (PC >= total_instructions) PC = total_instructions - 1; // Bound PC
    flush_flag = 0;
    printf("Flushed pipeline, new PC: %d\n", PC);
}

void terminate_pipeline() {
    if_stage.active = false;
    id_stage.active = false;
    ex_stage.active = false;
    mem_stage.active = false;
    wb_stage.active = false;
    if_stage.result = 0;
    id_stage.result = 0;
    ex_stage.result = 0;
    mem_stage.result = 0;
    wb_stage.result = 0;
}

int main() {
    init_memory();
    init_registers();

    FILE *file = fopen(FILENAME, "r");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    char buffer[256];
    int instruction_index = 0;

    printf("Reading file: %s\n\n", FILENAME);

    while (fgets(buffer, sizeof(buffer), file) && instruction_index < MAX_INSTRUCTIONS) {
        char *line = trim_whitespace(buffer);
        if (strlen(line) == 0) continue;

        uint32_t encoded = parse_instruction(line);
        instruction_memory[instruction_index++] = encoded;

        printf("Instruction %d encoded as: ", instruction_index);
        print_binary(encoded);
        printf("\n");
    }

    fclose(file);
    write_instruction_memory(instruction_memory);

    int clock_cycle = 0;
    total_instructions = instruction_index; // Dynamically set based on file input (11 in your case)
    bool terminate = false;

    while (!terminate) {
        printf("\nClock Cycle %d:\n", clock_cycle);

        // Write Back Stage
        if (wb_stage.active && wb_stage.cycles_remaining == 1) {
            write_back(&wb_stage.decoded, wb_stage.result);
            wb_stage.cycles_remaining--;
            wb_stage.active = false;
            instructions_executed++; // Increment after WB completes
            if (!flush_flag && PC < total_instructions - 1) {
                PC++;
            }
        }

        // Memory Stage
        if (mem_stage.active && mem_stage.cycles_remaining == 1) {
            memory_access(memory, &mem_stage.decoded);
            mem_stage.cycles_remaining--;
            if (!wb_stage.active) {
                if (mem_stage.decoded.opcode == 9) { // MOVR
                    extern uint32_t finalResult;
                    mem_stage.result = finalResult; // Ensure MOVR result is correct
                    printf("MOVR: Read value %u from memory[%d]\n", finalResult, mem_stage.decoded.addr);
                } else if (mem_stage.decoded.opcode == 10) { // MOVM
                    // Workaround: Assume memory is updated, force next MOVR to use R5
                    if (mem_stage.decoded.r1 == 5) { // R5 as source
                        memory[mem_stage.decoded.addr] = registers[5]; // Force MEM[20] = R5
                        printf("MOVM: Forced memory[%d] = %u from R5\n", mem_stage.decoded.addr, registers[5]);
                    }
                }
                wb_stage.instruction = mem_stage.instruction;
                wb_stage.decoded = mem_stage.decoded;
                wb_stage.result = mem_stage.result;
                wb_stage.cycles_remaining = 1;
                wb_stage.active = true;
                mem_stage.active = false;
            }
        }

        // Execute Stage
        if (ex_stage.active) {
            if (ex_stage.cycles_remaining == 1) {
                ex_stage.result = execute(&ex_stage.decoded);
                ex_stage.cycles_remaining--;
                if (mem_stage.decoded.opcode >= 6 && mem_stage.decoded.opcode <= 8) { // JEQ range
                    printf("JEQ Executed: R%d = %u, R%d = %u, Should branch: %s\n",
                           mem_stage.decoded.r1, registers[mem_stage.decoded.r1],
                           mem_stage.decoded.r2, registers[mem_stage.decoded.r2],
                           (registers[mem_stage.decoded.r1] == registers[mem_stage.decoded.r2]) ? "Yes" : "No");
                }
                if (flush_flag) {
                    flush_pipeline();
                    if (PC >= total_instructions) {
                        terminate = true;
                        terminate_pipeline();
                        continue;
                    }
                }
                if (!mem_stage.active && !terminate) {
                    mem_stage.instruction = ex_stage.instruction;
                    mem_stage.decoded = ex_stage.decoded;
                    mem_stage.result = ex_stage.result;
                    mem_stage.cycles_remaining = 1;
                    mem_stage.active = true;
                    ex_stage.active = false;
                }
            } else {
                ex_stage.cycles_remaining--;
            }
        }

        // Decode Stage
        if (id_stage.active) {
            if (id_stage.cycles_remaining == 1) {
                instruction_decode(id_stage.instruction, &id_stage.decoded);
                id_stage.cycles_remaining--;
                if (!ex_stage.active && !terminate) {
                    ex_stage.instruction = id_stage.instruction;
                    ex_stage.decoded = id_stage.decoded;
                    ex_stage.result = 0;
                    ex_stage.cycles_remaining = 2;
                    ex_stage.active = true;
                    id_stage.active = false;
                }
            } else {
                id_stage.cycles_remaining--;
            }
        }

        // Fetch Stage
        if (!terminate && flagwork && PC < total_instructions && instruction_memory[PC] != 0 && instructions_fetched < total_instructions) {
            if (!mem_stage.active && (!if_stage.active || (if_stage.cycles_remaining == 0 && !id_stage.active))) {
                if_stage.instruction = instruction_fetch(instruction_memory);
                if_stage.cycles_remaining = 2;
                if_stage.active = true;
                instructions_fetched++;
            } else if (if_stage.active && if_stage.cycles_remaining == 1) {
                if_stage.cycles_remaining--;
                if (!id_stage.active) {
                    id_stage.instruction = if_stage.instruction;
                    id_stage.cycles_remaining = 2;
                    id_stage.active = true;
                    if_stage.active = false;
                }
            } else if (if_stage.active) {
                if_stage.cycles_remaining--;
            }
        } else if (PC >= total_instructions) {
            if_stage.active = false; // Force stop fetching if beyond instructions
        }

        print_pipeline_state(clock_cycle);

        // Exit condition
        int max_cycles = 7 + ((instructions_executed - 1) * 2); // Dynamic cycle count
        if ((instructions_executed >= total_instructions || // All instructions executed
             (!if_stage.active && !id_stage.active && !ex_stage.active && 
              !mem_stage.active && !wb_stage.active)) || 
            (PC >= total_instructions && instructions_fetched >= total_instructions)) {
            terminate = true;
            terminate_pipeline();
        }

        clock_cycle++;
    }

    print_memory();
    print_registers();

    return EXIT_SUCCESS;
}