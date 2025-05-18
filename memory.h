#ifndef MEMORY_H
#define MEMORY_H

#define MEMORY_SIZE 2048
#define MAX_INSTRUCTIONS 1024

extern uint32_t memory[MEMORY_SIZE];
extern uint32_t instruction_memory[MAX_INSTRUCTIONS];

void init_memory(void);

#endif // MEMORY_H