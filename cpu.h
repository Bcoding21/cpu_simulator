/**
 * Gedare Bloom
 * Temilola Oloyede
 * Daniel Erhabor
 * David Awogbemila
 * Brandon Cole
 * cpu.h
 * Definitions for the processor.
 */

#include <stdint.h>
#include <stdbool.h>


struct Control {
	bool mem_to_reg;
	bool reg_write;
	bool mem_read;
	bool mem_write;
	bool branch;
	short alu_op;
};

struct RegDst_MUX {
	bool value;
};

struct ALUSrc_MUX {
	bool value;
};

struct MemtoReg_MUX {
	bool value;
};

struct cpu_context {
	uint32_t PC;
	uint32_t GPR[32];
	short ALU_CONTORL;
	struct Control CNTRL;
	struct RegDst_MUX regMUX;
	struct ALUSrc_MUX aluMUX;
	struct MemtoReg_MUX memToRegMUX;
};

extern struct cpu_context cpu_ctx;

struct IF_ID_buffer {
	uint32_t instruction;
	uint32_t next_pc;
};


struct ID_EX_buffer {
	bool alu_control;
	uint32_t read_data_1, read_data_2;

};

struct EX_MEM_buffer {
	uint32_t alu_result;
	uint32_t branch_result;
	uint32_t write_data;

};

struct MEM_WB_buffer {
	uint32_t write_data;
};

// Register File
struct REG_FILE_input {
	uint32_t read_reg_1, read_reg_2, write_reg, write_data;
	bool reg_write;
};

struct REG_FILE_output {
	uint32_t read_data_1, read_data_2;
};

// ALU I/O
struct ALU_INPUT {
	uint32_t input_1, input_2;
	short alu_control;
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
void setMultiplexors(short);




