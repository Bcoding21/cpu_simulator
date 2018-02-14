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
#include <stdlib.h>
struct cpu_context cpu_ctx;

int fetch( struct IF_ID_buffer *out )
{
	instructionMemory(cpu_ctx.PC, out);
	return 0;
}

int decode( struct IF_ID_buffer *in, struct ID_EX_buffer *out )
{
	struct REG_FILE_input* registerInputs = (struct REG_FILE_input*) malloc(sizeof(struct REG_FILE_input)); // holds inputs

	int opcode = in->instruction >> 26; // gets bits 31:26

	setControl(opcode); // set outputs for control units
	setALUControl(opcode); // set alu control output
	setMultiplexors(opcode); // set all multiplexor  states

	// set inputs
	registerInputs->read_reg_1 = (in->instruction >> 20) & 0x1F; // get bits 25:31(rs)
	registerInputs->read_reg_2 = (in->instruction >> 15) & 0x1F; // get bits 20:16(rt)
	registerInputs->write_reg = (in->instruction >> 10) & 0x1F; // gets bits 15:11(rd)
	registerInputs->reg_write = cpu_ctx.CNTRL.reg_write; // set register control signal
	uint32_t sign_extended_val = in->instruction & 0x7FFF; // 15:0 // address

	struct REG_FILE_output* registerOutputs = (struct REG_FILE_input*) malloc(sizeof(struct REG_FILE_input)); // holds outputs

	registerFile(registerInputs, registerOutputs);

    // set outputs
	out->read_data_1 = registerOutputs->read_data_1;
	out->read_data_2 = (cpu_ctx.aluMUX.value) ? sign_extended_val : registerOutputs->read_data_2; // if 1 then set to 16 bit sign extended. else set to output of read reg 2
    out->alu_control = in->instruction & 0x1F; // bits 5:0 (funct)
	return 0;
}

int execute( struct ID_EX_buffer *in, struct EX_MEM_buffer *out )
{
    struct ALU_INPUT* alu_input = (struct ALU_INPUT*) malloc(sizeof(struct ALU_INPUT)); // holds inputs

    // inputs
    alu_input->input_1 = in->read_data_1;
    alu_input->input_2 = in->read_data_2;
    alu_input->alu_control = in->alu_control;

    struct ALU_OUTPUT* alu_output = (struct ALU_OUTPUT*) malloc(sizeof(struct ALU_OUTPUT)); // hold outputs

    alu(alu_input, alu_output);

    // set outputs
    out->alu_result = alu_output->alu_result;
    out->branch_result = alu_output->branch_result;
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

	// Calculate ALU control output
	return 0;
}
a
int alu(struct ALU_INPUT* alu_input, struct ALU_OUTPUT* out){

    // will replace with funct later
    if (alu_input->alu_control == 0){
        out->alu_result = alu_input->input_1 & alu_input->input_2;
    }

    else if(alu_input->alu_control == 1){
        out->alu_result = alu_input->input_1 | alu_input->input_2;
    }

    else if(alu_input->alu_control == 2){
        out->alu_result = alu_input->input_1 + alu_input->input_2;
    }

    else if(alu_input->alu_control == 7) {
        out->alu_result = alu_input->input_1 - alu_input->input_2;
    }

    else if(alu_input->alu_control == 8){
        uint32_t result = alu_input->input_1 - alu_input->input_2;
        out->alu_result = (result < 0) ?  1 : 0;
    }



    return 0;
}


int setControl(uint32_t opcode) { // sets control signal outputs
	if (opcode == 0) { // R-Format
		cpu_ctx.CNTRL.mem_to_reg = false;
		cpu_ctx.CNTRL.reg_write = true;
		cpu_ctx.CNTRL.mem_read = false;
		cpu_ctx.CNTRL.mem_write = false;
		cpu_ctx.CNTRL.branch = false;
		cpu_ctx.CNTRL.alu_op = 2;
	}

	else if (opcode == 35) { // LW
		cpu_ctx.CNTRL.mem_to_reg = true;
		cpu_ctx.CNTRL.reg_write = true;
		cpu_ctx.CNTRL.mem_read = true;
		cpu_ctx.CNTRL.mem_write = false;
		cpu_ctx.CNTRL.branch = false;
		cpu_ctx.CNTRL.alu_op = 0;
	}

	else if (opcode == 43) { // SW
		cpu_ctx.CNTRL.mem_to_reg = false;
		cpu_ctx.CNTRL.reg_write = false;
		cpu_ctx.CNTRL.mem_read = false;
		cpu_ctx.CNTRL.mem_write = true;
		cpu_ctx.CNTRL.branch = false;
		cpu_ctx.CNTRL.alu_op = 0;
	}

	else { // BEQ
		cpu_ctx.CNTRL.mem_to_reg = false;
		cpu_ctx.CNTRL.reg_write = false;
		cpu_ctx.CNTRL.mem_read = false;
		cpu_ctx.CNTRL.mem_write = false;
		cpu_ctx.CNTRL.branch = true;
		cpu_ctx.CNTRL.alu_op = 1;
	}
}


void setMultiplexors(short opcode) { // sets multiplexor states
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
