/**
 * Gedare Bloom
 * Temilola Oloyede
 * Daniel Erhabor
 * David Awogbemila
 * Brandon Cole.
 * cpu.c
 * Implementation of simulated processor.
 */

#include "cpu.h"
#include "memory.h"

struct cpu_context cpu_ctx;


int fetch( struct IF_ID_buffer *out )
{
	instructionMemory(cpu_ctx.PC, out);
	return 0;
}

int decode( struct IF_ID_buffer *in, struct ID_EX_buffer *out )
{
	struct REG_FILE_input* registerInputs = (struct REG_FILE_input*) malloc(sizeof(struct REG_FILE_input)); // carrys inputs in
	int opcode = in->instruction >> 26; // gets bits 31:26
	setControl(opcode); // set outputs for control units
	setMultiplexors(opcode); // set all multiplexor  states

	registerInputs->read_reg_1 = (in->instruction >> 20) & 0x1F; // get bits 25:31
	registerInputs->read_reg_2 = (in->instruction >> 15) & 0x1F; // get bits 20:16
	registerInputs->write_reg = (in->instruction >> 10) & 0x1F; // gets bits 15:11
	registerInputs->reg_write = cpu_ctx.CNTRL.reg_write; // set register control signal

	struct REG_FILE_output* registerOutputs = (struct REG_FILE_input*) malloc(sizeof(struct REG_FILE_input)); // carrys outputs out
	registerFile(registerInputs, registerOutputs);

	out->funct_code = in->instruction & 0x1F;
	out->read_data_1 = registerOutputs->read_data_1;
	out->read_data_2 = (cpu_ctx.aluMUX.value) ? in->instruction & 0x7FFF : registerOutputs->read_data_2; // if 1 then set to 16 bit sign exteded. else set to output of read reg 2
	return 0;
}

int execute( struct ID_EX_buffer *in, struct EX_MEM_buffer *out )
{

	return 0;
}

int memory( struct EX_MEM_buffer *in, struct MEM_WB_buffer *out )
{
	return 0;
}

int writeback( struct MEM_WB_buffer *in )
{
	return 0;
}

int instructionMemory(uint32_t address, struct IF_ID_buffer *out) {
	int *addressPtr = address;
	out->instruction = *addressPtr;
	return 0;
}

int registerFile(struct REG_FILE_input* input, struct REG_FILE_output* output) {
	output->read_data_1 = cpu_ctx.GPR[input->read_reg_1];
	output->read_data_2 = cpu_ctx.GPR[input->read_reg_2];
}






int setControl(uint32_t opcode) {
	if (opcode == 0) { // R-Format
		cpu_ctx.CNTRL.mem_to_reg = 0;
		cpu_ctx.CNTRL.reg_write = 1;
		cpu_ctx.CNTRL.mem_read = 0;
		cpu_ctx.CNTRL.mem_write = 0;
		cpu_ctx.CNTRL.branch = 0;
		cpu_ctx.CNTRL.alu_op = 1;
	}

	else if (opcode == 35) { // LW
		cpu_ctx.CNTRL.mem_to_reg = 1;
		cpu_ctx.CNTRL.reg_write = 1;
		cpu_ctx.CNTRL.mem_read = 1;
		cpu_ctx.CNTRL.mem_write = 0;
		cpu_ctx.CNTRL.branch = 0;
		cpu_ctx.CNTRL.alu_op = 0;
	}

	else if (opcode == 43) { // SW
		cpu_ctx.CNTRL.mem_to_reg = 0;
		cpu_ctx.CNTRL.reg_write = 0;
		cpu_ctx.CNTRL.mem_read = 0;
		cpu_ctx.CNTRL.mem_write = 1;
		cpu_ctx.CNTRL.branch = 0;
		cpu_ctx.CNTRL.alu_op = 0;
	}

	else { // BEQ
		cpu_ctx.CNTRL.mem_to_reg = 0;
		cpu_ctx.CNTRL.reg_write = 0;
		cpu_ctx.CNTRL.mem_read = 0;
		cpu_ctx.CNTRL.mem_write = 0;
		cpu_ctx.CNTRL.branch = 1;
		cpu_ctx.CNTRL.alu_op = 0;
	}
}


void setMultiplexors(short opcode) {
	if (opcode == 0) {
		cpu_ctx.regMUX.value = true;
		cpu_ctx.aluMUX.value = false;
		cpu_ctx.memToRegMUX.value = false;

	}

	else if (opcode == 35) {
		cpu_ctx.regMUX.value = false;
		cpu_ctx.aluMUX.value = true;
		cpu_ctx.memToRegMUX.value = true;
	}

	else {
		cpu_ctx.regMUX.value = true;
		cpu_ctx.aluMUX.value = false;
		cpu_ctx.memToRegMUX.value = false;
	}

}
