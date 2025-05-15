#ifndef PIPELINE_H
#define PIPELINE_H

#include <stdint.h>

// Function declarations
uint32_t instruction_fetch(uint32_t instruction_memory[], uint32_t *pc, uint32_t *if_id_register, int cycle);
void instruction_decode(uint32_t if_id_register, uint32_t *id_ex_register, int cycle);
void execute(uint32_t id_ex_register, uint32_t *ex_mem_register, int cycle);
void memory_access(uint32_t ex_mem_register, uint32_t *mem_wb_register, int cycle);
void write_back(uint32_t mem_wb_register, int cycle);

// Mock memory functions
uint32_t read_memory(uint32_t address);
void write_memory(uint32_t address, uint32_t value);

#endif // PIPELINE_H