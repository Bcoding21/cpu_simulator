/**
 * Gedare Bloom
 * Temilola Oloyede
 * Daniel Erhabor
 * David Awogbemila
 * Brandon Cole
 * Michaun Pierre
 * cpu.h
 * Definitions for the processor.
 */

#include <stdint.h>
#include <stdbool.h>
#define ENABLE_L1_CACHES

struct Control {
	bool reg_write;
	bool mem_read;
	bool mem_write;
	bool branch;
	bool mem_to_reg;
	bool alu_source;
	bool reg_dst;
	uint32_t alu_op;
	bool jump;
	bool jump_register;
};

uint32_t MULTIPLEXOR(bool selector, uint32_t HIGH_INPUT, uint32_t LOW_INPUT);

enum InstructionType{
	LOAD_WORD, STORE_WORD, BRANCH, R_TYPE
};

enum InstructionFormat{
	R_FORMAT, I_FORMAT, J_FORMAT, NO_OP
};

struct cpu_context {
	uint32_t PC;
	uint32_t GPR[32];
	struct Control CNTRL;
	bool interrupt;
	int stall_count;
};

// block is used to mean each row
// the data field is the block content of each row
struct Block {
	uint32_t data[4]; //each block should contain 4 words
	int offset;
	int tag;
	int index;
	bool valid;
	bool dirty;
};

struct Set {
	struct Block block_array[4];
	int lru_states[4];
	int fill_extent;// to handl einitial phase of compulsory misses
	//convention for LRU determination 0 -> MRU... 3 -> LRU
};

extern struct cpu_context cpu_ctx;

// will change name to l1_data_cache
extern struct Set l1_data_cache[32];
//declaring struct for instruction cache
extern struct Block L1_instruction_cache[128];

struct IF_ID_buffer {
	uint32_t instruction;
	uint32_t pc_plus_4;
};

struct ID_EX_buffer {
	bool reg_write;
	bool mem_read;
	bool mem_write;
	bool branch;
	bool mem_to_reg;
	bool alu_source;
	bool reg_dst;
	bool jump;
	bool jump_register;
	uint32_t jump_target_address;
	uint32_t alu_op, pc_plus_4;
	short funct, opcode, shamt;
	uint32_t read_data_1, read_data_2, immediate;
	uint32_t RS_index, RT_index, RD_index;      // RS_index is needed in the executed stage for forwarding for data hazards
	bool interrupt;
};

struct EX_MEM_buffer {
	bool reg_write;
	bool branch;
	bool mem_write;
	bool mem_read;
	bool mem_to_reg;
	bool jump;
	bool jump_register;
	bool branch_result;
	uint32_t branch_target;
	uint32_t alu_result;
	uint32_t write_reg_index;
	uint32_t pc_plus_4;
	uint32_t jump_target_address;
	uint32_t read_data_2;
	bool interrupt;
};

struct MEM_WB_buffer {
	bool reg_write;
	bool mem_to_reg;
	bool jump;
	bool jump_register;
	uint32_t mem_read_data;
	uint32_t alu_result;
	uint32_t write_reg_index;
	bool interrupt;
	//	reg_write_data and alu_result_data have to both be present for the WB stage to decide whether to wuse
	// data from memory or alu result based on mem_to_reg value when writin gto register
};

// Stages
int fetch( struct IF_ID_buffer *out);
int decode( struct IF_ID_buffer *in, struct ID_EX_buffer *out );
int execute( struct ID_EX_buffer *in, struct EX_MEM_buffer *out );
int memory( struct EX_MEM_buffer *in, struct MEM_WB_buffer *out );
int writeback( struct MEM_WB_buffer *in );


// Register File I/O
struct REG_FILE_input {
	uint32_t read_reg_1, read_reg_2, write_reg, write_data;
	bool reg_write;
};

struct REG_FILE_output {
	uint32_t read_data_1, read_data_2;
};

// ALU I/O
struct ALU_INPUT {
	uint32_t input_1, input_2, immediate;
	short funct, opcode, shamt;
};

struct ALU_OUTPUT {
	uint32_t alu_result;
	bool branch_result;
};


// Data Memory
struct MEM_INPUT {
	uint32_t address, write_data;
	bool data_memory;
};

// Stages
int fetch( struct IF_ID_buffer *out);
int decode( struct IF_ID_buffer *in, struct ID_EX_buffer *out );
int execute( struct ID_EX_buffer *in, struct EX_MEM_buffer *out );
int memory( struct EX_MEM_buffer *in, struct MEM_WB_buffer *out );
int writeback( struct MEM_WB_buffer *in );

// Major Components
int instructionMemory(uint32_t address, struct IF_ID_buffer *out);
int registerFile(struct REG_FILE_input *in, struct REG_FILE_output *out);
int alu(struct ALU_INPUT* in, struct ALU_OUTPUT* out);

// Other Helper
int setControl(uint32_t);

// Helpers
int instructionMemory(uint32_t address, struct IF_ID_buffer *out);
int registerFile(struct REG_FILE_input*, struct REG_FILE_output*);
int alu(struct ALU_INPUT* in, struct ALU_OUTPUT* out);
int setControlState(short);
void setInstructionFormat(short);
void setMultiplexors();

