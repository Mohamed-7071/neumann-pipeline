#include <stdint.h>

// Instruction structure definition
typedef struct {
    uint8_t opcode;
    uint8_t r1;
    uint8_t r2;
    uint8_t r3;
    uint32_t shamt;
    int32_t imm;
    uint32_t addr;
} Instruction;

// Function declarations
void safe_register_write(uint8_t reg, uint32_t value, const char* stage);
uint32_t instruction_fetch(uint32_t *memory); // PC increment removed from here
void instruction_decode(uint32_t binary_instruction, Instruction *instr);
uint32_t execute(Instruction *instr, uint32_t instruction_address);
void memory_access(uint32_t *memory, Instruction *instr);
void write_back(Instruction *instr, uint32_t result);
char *trim_whitespace(char *str);
uint32_t parse_instruction(const char *line);
void print_binary(uint32_t value);
void write_instruction_memory(uint32_t *instruction_array);
void print_memory(void);

// Global variables
extern uint32_t instruction;
extern int flush_flag;
extern uint32_t branch_target;
extern uint32_t movr_address;
extern uint32_t finalResult;
extern bool flagwork;

