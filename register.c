#include "registers.h"
#include <stdio.h>

uint32_t registers[NUM_REGISTERS];
uint32_t PC = 0;

void init_registers() {
    for (int i = 0; i < NUM_REGISTERS; i++) {
        registers[i] = 0;
    }
    registers[0] = 0;  // R0 is hard-wired to zero
    PC = 0;
}

uint32_t get_register(uint8_t index) {
    if (index >= NUM_REGISTERS) {
        fprintf(stderr, "Register index out of bounds: R%d\n", index);
        return 0;
    }
    return registers[index];
}

void set_register(uint8_t index, uint32_t value) {
    if (index == 0) return;  // R0 is hard-wired to zero
    if (index >= NUM_REGISTERS) {
        fprintf(stderr, "Register index out of bounds: R%d\n", index);
        return;
    }
    registers[index] = value;
}

void print_registers() {
    for (int i = 0; i < NUM_REGISTERS; i++) {
        printf("R%-2d = %u\n", i, registers[i]);
    }
    printf("PC  = %u\n", PC);
}
