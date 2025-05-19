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

// --- Add pending flush state ---
bool pending_flush = false;
uint32_t branch_flush_target = 0;

// Pipeline stage structures
typedef struct {
    uint32_t instruction;
    Instruction decoded;
    int cycles_remaining;
    bool active;
    uint32_t result; // Separate field for execution result
    uint32_t instruction_address; // <-- Add this field to track instruction index
} PipelineStage;

PipelineStage if_stage = {0, {0}, 2, false, 0, 0};
PipelineStage id_stage = {0, {0}, 2, false, 0, 0};
PipelineStage ex_stage = {0, {0}, 2, false, 0, 0};
PipelineStage mem_stage = {0, {0}, 1, false, 0, 0};
PipelineStage wb_stage = {0, {0}, 1, false, 0, 0};

int total_instructions = 0;
int instructions_fetched = 0; // Track number of instructions fetched
int instructions_executed = 0; // Track number of instructions completed

// Helper function to check for data hazards (RAW)
bool has_data_hazard(Instruction *curr, PipelineStage *ex, PipelineStage *mem, PipelineStage *wb) {
    if (!curr) return false;
    // Source registers for the current instruction
    uint8_t src_regs[3] = {0, 0, 0};
    int src_count = 0;
    int opcode = curr->opcode;
    if (opcode <= 5) { // R-type: src = r2, r3
        src_regs[0] = curr->r2;
        src_regs[1] = curr->r3;
        src_count = 2;
    } else if (opcode == 7) { // JEQ: src = r1, r2
        src_regs[0] = curr->r1;
        src_regs[1] = curr->r2;
        src_count = 2;
    } else if (opcode == 8) { // XORI: src = r1
        src_regs[0] = curr->r1;
        src_count = 1;
    } else if (opcode == 9) { // MOVR: src = r2
        src_regs[0] = curr->r2;
        src_count = 1;
    } else if (opcode == 10) { // MOVM: src = r1 (store), r2 (address)
        src_regs[0] = curr->r1;
        src_regs[1] = curr->r2;
        src_count = 2;
    }
    // Check EX, MEM, WB for RAW hazard
    PipelineStage *stages[3] = {ex, mem, wb};
    for (int i = 0; i < 3; i++) {
        if (stages[i]->active) {
            int dest_opcode = stages[i]->decoded.opcode;
            uint8_t dest = 0;
            if (dest_opcode <= 6 || dest_opcode == 8 || dest_opcode == 9) {
                dest = stages[i]->decoded.r1;
            } else {
                dest = 0;
            }
            if (dest != 0) {
                for (int j = 0; j < src_count; j++) {
                    // Only skip hazard if src_regs[j] == 0 (R0)
                    if (src_regs[j] == 0) continue;
                    if (dest == src_regs[j]) {
                        return true;
                    }
                }
            }
        }
    }
    // Special case: MOVM->MOVR memory hazard (check both EX and MEM stages)
    if (curr->opcode == 9) { // MOVR in ID
        if (ex->active && ex->decoded.opcode == 10) {
            uint32_t movm_addr = registers[ex->decoded.r2] + ex->decoded.imm;
            uint32_t movr_addr = registers[curr->r2] + curr->imm;
            if (movm_addr == movr_addr) {
                return true;
            }
        }
        if (mem->active && mem->decoded.opcode == 10) {
            uint32_t movm_addr = registers[mem->decoded.r2] + mem->decoded.imm;
            uint32_t movr_addr = registers[curr->r2] + curr->imm;
            if (movm_addr == movr_addr) {
                return true;
            }
        }
    }
    // Additional fix: Stall MOVM if its source register (r1) is being written by EX, MEM, or WB
    if (curr->opcode == 10) { // MOVM in ID
        for (int i = 0; i < 3; i++) {
            if (stages[i]->active) {
                int dest_opcode = stages[i]->decoded.opcode;
                uint8_t dest = 0;
                if (dest_opcode <= 6 || dest_opcode == 8 || dest_opcode == 9) {
                    dest = stages[i]->decoded.r1;
                } else {
                    dest = 0;
                }
                // Only check if dest is not R0 and matches MOVM's r1 (store value)
                if (dest != 0 && dest == curr->r1) {
                    return true;
                }
            }
        }
    }
    return false;
}

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
    // Flush all pipeline stages
    if_stage.active = false;
    if_stage.cycles_remaining = 2;
    if_stage.instruction = 0;
    if_stage.result = 0;

    id_stage.active = false;
    id_stage.cycles_remaining = 2;
    id_stage.instruction = 0;
    memset(&id_stage.decoded, 0, sizeof(Instruction));
    id_stage.result = 0;

    ex_stage.active = false;
    mem_stage.active = false;
    wb_stage.active = false;
    ex_stage.result = 0;
    mem_stage.result = 0;
    wb_stage.result = 0;

    // Set PC to branch_target (no -1) so next fetch is correct
    uint32_t old_pc = PC;
    PC = branch_target;
    if (PC >= total_instructions) PC = total_instructions - 1;
    flush_flag = 0;
    printf("[FLUSH] Pipeline flushed. Old PC: %u, New PC: %u (Branch Target: %u)\n", old_pc, PC, branch_target);
}

void flush_pipeline_after_wb() {
    if_stage.active = false;
    if_stage.cycles_remaining = 2;
    if_stage.instruction = 0;
    if_stage.result = 0;

    id_stage.active = false;
    id_stage.cycles_remaining = 2;
    id_stage.instruction = 0;
    memset(&id_stage.decoded, 0, sizeof(Instruction));
    id_stage.result = 0;

    ex_stage.active = false;
    mem_stage.active = false;
    wb_stage.active = false;
    ex_stage.result = 0;
    mem_stage.result = 0;
    wb_stage.result = 0;

    uint32_t old_pc = PC;
    PC = branch_flush_target;
    pending_flush = false;
    flush_flag = 0;
    printf("[FLUSH] Pipeline flushed after WB. Old PC: %u, New PC: %u (Branch Target: %u)\n", old_pc, PC, branch_flush_target);
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
    bool stall = false; // Add stall flag

    while (!terminate) {
        printf("\nClock Cycle %d:\n", clock_cycle);

        // Write Back Stage
        if (wb_stage.active && wb_stage.cycles_remaining == 1) {
            write_back(&wb_stage.decoded, wb_stage.result);
            wb_stage.cycles_remaining--;
            wb_stage.active = false;
            instructions_executed++; // Increment after WB completes
            // --- If a flush is pending, flush pipeline after WB ---
            if (pending_flush) {
                flush_pipeline_after_wb();
                // After flush, skip rest of this cycle to avoid fetching/advancing pipeline in same cycle
                print_pipeline_state(clock_cycle);
                clock_cycle++;
                continue;
            }
        }

        // Memory Stage
        if (mem_stage.active && mem_stage.cycles_remaining == 1) {
            memory_access(memory, &mem_stage.decoded);
            mem_stage.cycles_remaining--;
            if (!wb_stage.active) {
                if (mem_stage.decoded.opcode == 9) { // MOVR
                    extern uint32_t finalResult;
                    mem_stage.result = finalResult; // Value loaded from memory
                    printf("MOVR: Read value %u from memory[%d]\n", finalResult, registers[mem_stage.decoded.r2] + mem_stage.decoded.imm);
                }
                wb_stage.instruction = mem_stage.instruction;
                wb_stage.instruction_address = mem_stage.instruction_address; // propagate address
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
                ex_stage.result = execute(&ex_stage.decoded, ex_stage.instruction_address); // pass correct address
                ex_stage.cycles_remaining--;
                // --- Branch/Jump logic: set pending flush if needed ---
                if (flush_flag) {
                    pending_flush = true;
                    branch_flush_target = branch_target;
                    // Do NOT flush now; wait until WB of this instruction
                }
                if (!mem_stage.active && !terminate) {
                    mem_stage.instruction = ex_stage.instruction;
                    mem_stage.instruction_address = ex_stage.instruction_address; // propagate address
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

        // Data Hazard Detection (stall logic)
        stall = false;
        Instruction temp_decoded = {0};
        if (id_stage.active && id_stage.cycles_remaining == 1) {
            // Decode instruction to check hazard
            instruction_decode(id_stage.instruction, &temp_decoded);
            if (has_data_hazard(&temp_decoded, &ex_stage, &mem_stage, &wb_stage)) {
                stall = true;
                printf("[STALL] Data hazard detected. Stalling pipeline.\n");
            }
        }
        // Decode Stage
        if (id_stage.active) {
            instruction_decode(id_stage.instruction, &id_stage.decoded);
            if (id_stage.cycles_remaining == 1 && !stall) {
                id_stage.cycles_remaining--;
                if (!ex_stage.active && !terminate) {
                    ex_stage.instruction = id_stage.instruction;
                    ex_stage.instruction_address = id_stage.instruction_address; // propagate address
                    ex_stage.decoded = id_stage.decoded;
                    ex_stage.result = 0;
                    ex_stage.cycles_remaining = 2;
                    ex_stage.active = true;
                    id_stage.active = false;
                }
            } else if (!stall) {
                id_stage.cycles_remaining--;
            }
        }

        // Fetch Stage
        // Only allow IF if MEM is not active this cycle
        if (!terminate && flagwork && !stall && !flush_flag && PC < total_instructions && instruction_memory[PC] != 0 && !mem_stage.active) {
            if (!if_stage.active) {
                if_stage.instruction = instruction_fetch(instruction_memory);
                if_stage.instruction_address = PC; // Store the address before incrementing PC
                if_stage.cycles_remaining = 2;
                if_stage.active = true;
                instructions_fetched++;
                PC++; // PC is incremented here after a successful fetch
            }
        }
        // IF to ID progression (only if not stalling)
        if (if_stage.active && if_stage.cycles_remaining == 1 && !stall) {
            if_stage.cycles_remaining--;
            if (!id_stage.active) {
                id_stage.instruction = if_stage.instruction;
                id_stage.instruction_address = if_stage.instruction_address; // propagate address
                id_stage.cycles_remaining = 2;
                id_stage.active = true;
                if_stage.active = false;
            }
        } else if (if_stage.active && !stall) {
            if_stage.cycles_remaining--;
        }
        // If stalling, IF and ID hold their state (do not decrement cycles_remaining)

        print_pipeline_state(clock_cycle);

        // Exit condition
        // Stop fetching new instructions once all have been fetched, but let pipeline drain
        bool pipeline_empty = !if_stage.active && !id_stage.active && !ex_stage.active && !mem_stage.active && !wb_stage.active;
        bool all_fetched = (instructions_fetched >= total_instructions || PC >= total_instructions);
        if (pipeline_empty && all_fetched) {
            terminate = true;
            terminate_pipeline();
        }
        // Keep the hard max cycle count as a safety net
        if (clock_cycle > 1000) { // Hard max cycle count to prevent infinite loop
            printf("\n[ERROR] Max cycle count reached. Terminating pipeline.\n");
            terminate = true;
            terminate_pipeline();
        }

        clock_cycle++;
    }

    print_memory();
    print_registers();

    return EXIT_SUCCESS;
}