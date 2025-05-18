#ifndef INSTRUCTION_PARSER_H
#define INSTRUCTION_PARSER_H

#include <stdint.h>

// Maximum number of instructions in the instruction memory
#define MAX_INSTRUCTIONS 1024

// Instruction memory array declaration (extern for linking)
extern uint32_t instruction_memory[MAX_INSTRUCTIONS];

// Function to trim whitespace from a string (returns pointer to trimmed string)
char *trim_whitespace(char *str);

// Convert register string like "R5" to int 5; returns -1 if invalid
int reg_to_int(const char *reg);

// Parse immediate value string to int32_t
int32_t parse_immediate(const char *imm_str);

// Encode R-type instruction
uint32_t encode_r_type(int opcode, int r1, int r2, int r3, int shamt);

// Encode I-type instruction
uint32_t encode_i_type(int opcode, int r1, int r2, int32_t imm);

// Encode J-type instruction
uint32_t encode_j_type(int opcode, int address);

// Print a 32-bit value in binary (for debugging)
void print_binary(uint32_t value);

// Parse a single assembly instruction line and return encoded instruction
uint32_t parse_instruction(const char *line);

#endif // INSTRUCTION_PARSER_H
