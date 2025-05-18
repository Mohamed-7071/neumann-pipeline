#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "registers.h"

#include "memory.h"
#include "funcs.h"



#define FILENAME "program.txt"
#define MAX_INSTRUCTIONS 1024

extern uint32_t instruction_memory[MAX_INSTRUCTIONS];
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
   