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
uint32_t instruction_fetch(uint32_t *instruction_memory);
void instruction_decode(uint32_t binary_instruction, Instruction *instr);
uint32_t execute(Instruction *instr);
void memory_access(uint32_t *memory, Instruction *instr);
void write_back(Instruction *instr, uint32_t result);

// Global variables
extern uint32_t instruction;
extern int flush_flag;
extern uint32_t branch_target;
extern uint32_t movr_address;
extern uint32_t finalResult;

