// filepath: e:\neumann-pipeline\main.h
#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

// Declare any global variables or functions you want to share
extern uint32_t registers[32];

uint32_t read_memory(uint32_t address);
void write_memory(uint32_t address, uint32_t value);

#endif // MAIN_H