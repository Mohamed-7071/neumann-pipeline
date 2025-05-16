#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

extern uint32_t registers[32];
uint32_t get_register(uint32_t reg);
void set_register(uint32_t reg, uint32_t value);

#endif // REGISTERS_H