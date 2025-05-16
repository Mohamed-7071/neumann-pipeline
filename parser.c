
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "memory.c" 

#define FILENAME "program.txt"
#define MAX_INSTRUCTIONS 1024

uint32_t instruction_memory[MAX_INSTRUCTIONS]; 


char *trim_whitespace(char *str) {
    while (isspace(*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = '\0';
    return str;
}

// bt convert register string to int ex R5 --> 5
int reg_to_int(const char *reg) {
    if (reg[0] != 'R') return -1;
    return atoi(reg + 1);
}

int32_t parse_immediate(const char *imm_str) {
    return atoi(imm_str); 
}


uint32_t encode_r_type(int opcode, int r1, int r2, int r3, int shamt) {
    return (opcode << 28) | (r1 << 23) | (r2 << 18) | (r3 << 13) | (shamt & 0x1FFF);
}


uint32_t encode_i_type(int opcode, int r1, int r2, int32_t imm) {
    return (opcode << 28) | (r1 << 23) | (r2 << 18) | (imm & 0x3FFFF);
}


uint32_t encode_j_type(int opcode, int address) {
    return (opcode << 28) | (address & 0x0FFFFFFF);
}


void print_binary(uint32_t value) {
    for (int i = 31; i >= 0; i--) {
        printf("%d", (value >> i) & 1);
    }
}


uint32_t parse_instruction(const char *line) {
    char instr[10], op1[10], op2[10], op3[10];
    int opcode = -1;
    sscanf(line, "%s %s %s %s", instr, op1, op2, op3);

    for (int i = 0; instr[i]; i++)
        instr[i] = toupper(instr[i]);
    // R
    if (strcmp(instr, "ADD") == 0)
        opcode = 0;
    else if (strcmp(instr, "SUB") == 0)
        opcode = 1;
    else if (strcmp(instr, "MUL") == 0)
        opcode = 2;
    else if (strcmp(instr, "AND") == 0)
        opcode = 3;
    else if (strcmp(instr, "LSL") == 0)
        opcode = 4;
    else if (strcmp(instr, "LSR") == 0)
        opcode = 5;
    // I
    else if (strcmp(instr, "MOVI") == 0)
        opcode = 6;
    else if (strcmp(instr, "JEQ") == 0)
        opcode = 7;
    else if (strcmp(instr, "XORI") == 0)
        opcode = 8;
    else if (strcmp(instr, "MOVR") == 0)
        opcode = 9;
    else if (strcmp(instr, "MOVM") == 0)
        opcode = 10;
    // J
    else if (strcmp(instr, "JMP") == 0)
        opcode = 11;
    else
    {
        fprintf(stderr, "Unknown instruction: %s\n", instr);
        exit(EXIT_FAILURE);
    }

    switch (opcode) {
        case 0: case 1: case 2: case 3:
            return encode_r_type(opcode, reg_to_int(op1), reg_to_int(op2), reg_to_int(op3), 0);
        case 4: case 5:
            return encode_r_type(opcode, reg_to_int(op1), reg_to_int(op2), 0, atoi(op3));
        case 6:
            return encode_i_type(opcode, reg_to_int(op1), 0, parse_immediate(op2));
        case 7:
            return encode_i_type(opcode, reg_to_int(op1), reg_to_int(op2), parse_immediate(op3));
        case 8:
            return encode_j_type(opcode, parse_immediate(op1));
        case 9: case 10:
            return encode_i_type(opcode, reg_to_int(op1), reg_to_int(op2), parse_immediate(op3));
        case 11:
            return 0x00000000;
        default:
            fprintf(stderr, "Invalid opcode: %d\n", opcode);
            exit(EXIT_FAILURE);
    }
}

int main() {
    init_memory(); // btsahel 3alaya 7etet el write fel memory file

    
    

    FILE *file = fopen(FILENAME, "r");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    char buffer[256];
    int instruction_index = 0;

    printf("Reading file: %s\n\n", FILENAME);

    while (fgets(buffer, sizeof(buffer), file)) {
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


     print_memory();

    return EXIT_SUCCESS;
}
    
