#include "registers.h"
#include <stdio.h>
#include <stdint.h>

#define NUM_REGISTERS 32

// Register file
uint32_t registers[NUM_REGISTERS] = {0}; // R0 to R31
uint32_t PC = 0;                          // Program Counter

// Initialize all registers to 0 (R0 is always zero)
void init_registers()
{
    for (int i = 0; i < NUM_REGISTERS; i++)
    {
        registers[i] = 0;
    }
    registers[0] = 0; // Hard-wired R0
    PC = 0;
}

// Get the value of a register (R0 always returns 0)
uint32_t get_register(uint8_t index)
{
    if (index >= NUM_REGISTERS)
    {
        fprintf(stderr, "Register index out of bounds: R%d\n", index);
        return 0;
    }
    if (index == 0)
        return 0; // R0 always 0
    return registers[index];
}

// Set the value of a register (R0 is read-only)
void set_register(uint8_t index, uint32_t value)
{
    if (index >= NUM_REGISTERS)
    {
        fprintf(stderr, "Register index out of bounds: R%d\n", index);
        return;
    }
    if (index == 0)
    {
        registers[0] = 0; // R0 remains 0
        return;
    }
    registers[index] = value;
}

// Print all register values
void print_registers()
{
    for (int i = 0; i < NUM_REGISTERS; i++)
    {
        printf("R%-2d = %u\n", i, registers[i]);
    }
    printf("PC  = %u\n", PC);
}
