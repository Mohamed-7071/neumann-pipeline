#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

#define NUM_REGISTERS 32

extern uint32_t registers[NUM_REGISTERS];
extern uint32_t PC;

void init_registers();
uint32_t get_register(uint8_t index);
void set_register(uint8_t index, uint32_t value);
void print_registers();

#endif
