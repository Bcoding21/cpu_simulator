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
#include <stdio.h>
int instructionMemory(uint32_t address, struct IF_ID_buffer *out);
int registerFile(struct REG_FILE_input* input, struct REG_FILE_output* output);
int alu(struct ALU_INPUT* alu_input, struct ALU_OUTPUT* out);

struct cpu_context cpu_ctx;

int fetch(struct IF_ID_buffer *out )
{
	instructionMemory(cpu_ctx.PC, out);
	return 0;
}

int decode( struct IF_ID_buffer *in, struct ID_EX_buffer *out )
{
	short opcode = in->instruction >> 26; // gets bits 31:26

	setControlState(opcode); // set outputs for control units
	setMultiplexors(); // set all multiplexor  states

	// set inputs
    struct REG_FILE_input* registerInputs = (struct REG_FILE_input*) malloc(sizeof(struct REG_FILE_input)); // holds inputs
	registerInputs->read_reg_1 = (in->instruction >> 21) & 0x1F; // get bits 25:21 (rs)
	registerInputs->read_reg_2 = (in->instruction >> 16) & 0x1F; // get bits 20:16(rt)

	// Write register will depend on the RegDst multiplexor      gets bits 15:11(rd)                get bits 20:16(rt)
	registerInputs->write_reg = MULTIPLEXOR(cpu_ctx.RegDst_MUX, (in->instruction >> 11) & 0x1F, registerInputs->read_reg_2);
//	registerInputs->write_reg = (in->instruction >> 11) & 0x1F; // gets bits 15:11(rd)
	uint32_t sign_extended_val = in->instruction & 0x7FFF; // 15:0 // address
	registerInputs->reg_write = cpu_ctx.CNTRL.reg_write; // set register control signal

	struct REG_FILE_output* registerOutputs = (struct REG_FILE_output*) malloc(sizeof(struct REG_FILE_output)); // holds outputs

	registerFile(registerInputs, registerOutputs);


	out->read_data_1 = registerOutputs->read_data_1;
	out->read_data_2 = (cpu_ctx.ALUSrc_MUX) ? sign_extended_val : registerOutputs->read_data_2; // if 1 then set to 16 bit sign extended. else set to output of read reg 2
 	out->funct = in->instruction & 0x3F; // bits 5:0 funct
	out->opcode = opcode;

	printf("opcode: %d\n", out->opcode);
	printf("read_data_1: %d\n", out->read_data_1);
	printf("read_data_2: %d\n", out->read_data_2);
	printf("funct: %d\n", out->funct);
	return 0;
}

int execute( struct ID_EX_buffer *in, struct EX_MEM_buffer *out )
{
    struct ALU_INPUT* alu_input = (struct ALU_INPUT*) malloc(sizeof(struct ALU_INPUT)); // holds inputs

    // inputs
    alu_input->input_1 = in->read_data_1;
    alu_input->input_2 = in->read_data_2;
	alu_input->funct = in->funct;
	alu_input->opcode = in->opcode;

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
	out->instruction = instruction_memory[address - 0x400000];
	// printf("Instruction got: %d\n", out->instruction);
	return 0;
}

int registerFile(struct REG_FILE_input* input, struct REG_FILE_output* output) {
	output->read_data_1 = cpu_ctx.GPR[input->read_reg_1];
	output->read_data_2 = cpu_ctx.GPR[input->read_reg_2];
	return 0;
}

int alu(struct ALU_INPUT* alu_input, struct ALU_OUTPUT* out){

	out->branch_result = 0;
	switch (cpu_ctx.instructionFormat){

		case R_FORMAT:

			if (alu_input->funct == 0x20){ // add
				out->alu_result = alu_input->input_1 + alu_input->input_2;
			}

			else if(alu_input->funct == 0x24){ // and
				out->alu_result = alu_input->input_1 & alu_input->input_2;
			}

			else if(alu_input->funct == 0x27){ // nor
				out->alu_result = ~(alu_input->input_1 | alu_input->input_2);
			}

			else if(alu_input->funct == 0x26){ // xor
				out->alu_result = alu_input->input_1 ^ alu_input->input_2;
			}

			else if(alu_input->funct == 0x2A){ // slt
				int result = alu_input->input_1 - alu_input->input_2;
				out->alu_result = (result < 0 ) ? 1 : 0;
			}

			else if(alu_input->funct == 0x00){ // sll
				out->alu_result = alu_input->input_1 << alu_input->input_2;
			}

			else if(alu_input->funct == 0x02){ // srl
				out->alu_result = alu_input->input_1 >> alu_input->input_2;
			}

			else if(alu_input->funct == 0x22){ // sub
				out->alu_result = alu_input->input_1 - alu_input->input_2;
			}

			else if (alu_input->funct == 0x03){ // sra
                int32_t result = alu_input->input_1;        // Make it signed to shift right arithmetic
                result = alu_input->input_1 >> alu_input->input_2;  // Shift right keeping the sign bit
                out->alu_result = result;  // Assign it back to out->alu_result to be consistent with its return value
			}
			else if (alu_input->funct == 0x25){
                out->alu_result = alu_input->input_1 | alu_input->input_2; // OP1 or OP2
			}

			break;

		case I_FORMAT:
			if (alu_input->opcode == 0x08){ // addi
				out->alu_result = alu_input->input_1 + alu_input->input_2;
			}

			else if (alu_input->opcode == 0x04){ // beq
				out->branch_result = (alu_input->input_1 == alu_input->input_2) ? true : false;
			}

			else if (alu_input->opcode == 0x05){ // bne
				out->branch_result = (alu_input->input_1 != alu_input->input_2) ? true : false;
			}

			else if (alu_input->opcode == 0x0F){ // lui
				out->alu_result = alu_input->input_2;
			}

			else if (alu_input->opcode == 0x23){ // lw
				out->alu_result = alu_input->input_1 + alu_input->input_2;
			}

			else if (alu_input->opcode == 0x0D){ // ori
				out->alu_result = alu_input->input_1 ^ alu_input->input_2;
			}

			else if (alu_input->opcode == 0x0A){ // slti
				out->alu_result = alu_input->input_1 << alu_input->input_2;
			}

			else if (alu_input->opcode == 0x2B){ // sw
				out->alu_result = alu_input->input_1 + alu_input->input_2;
			}

			else if (alu_input->opcode == 0x0E){ // xori
				out->alu_result = alu_input->input_1 ^ alu_input->input_2;
			}
			break;
	}
    return 0;
}

int setControlState(short opcode) { // sets control signal outputs

	setInstructionFormat(opcode);
	setMultiplexors();

	switch (cpu_ctx.instructionType){

		case R_TYPE:
			cpu_ctx.CNTRL.reg_write = true;
			cpu_ctx.CNTRL.mem_read = false;
			cpu_ctx.CNTRL.mem_write = false;
			cpu_ctx.CNTRL.branch = false;

		case LOAD_WORD:
			cpu_ctx.CNTRL.reg_write = true;
			cpu_ctx.CNTRL.mem_read = true;
			cpu_ctx.CNTRL.mem_write = false;
			cpu_ctx.CNTRL.branch = false;

		case STORE_WORD:
			cpu_ctx.CNTRL.reg_write = false;
			cpu_ctx.CNTRL.mem_read = false;
			cpu_ctx.CNTRL.mem_write = true;
			cpu_ctx.CNTRL.branch = false;

		case BRANCH:
			cpu_ctx.CNTRL.reg_write = false;
			cpu_ctx.CNTRL.mem_read = false;
			cpu_ctx.CNTRL.mem_write = false;
			cpu_ctx.CNTRL.branch = true;
			break;

		default:
			break;
	}

}

void setInstructionFormat(short opcode) {
	if (opcode == 0) {
		cpu_ctx.instructionFormat = R_FORMAT;
		cpu_ctx.instructionType = R_TYPE;
	}

	else if (opcode > 0 && opcode < 4){
		cpu_ctx.instructionFormat = I_FORMAT;
		cpu_ctx.instructionType = BRANCH;
	}

	else if (opcode > 3){
		cpu_ctx.instructionFormat = I_FORMAT;
		cpu_ctx.instructionType = (opcode == 0x2b) ? STORE_WORD : LOAD_WORD;
	}

	else{
		cpu_ctx.instructionFormat = NO_OP;
	}
}

void setMultiplexors() { // sets multiplexor states
	if (cpu_ctx.instructionType == R_TYPE) {
		cpu_ctx.RegDst_MUX = true;
		cpu_ctx.ALUSrc_MUX = false;
		cpu_ctx.MemtoReg_MUX = false;

	}

	else if (cpu_ctx.instructionType == LOAD_WORD) {
		cpu_ctx.RegDst_MUX = false;
		cpu_ctx.ALUSrc_MUX = true;
		cpu_ctx.MemtoReg_MUX = true;
	}

	else if (cpu_ctx.instructionType == STORE_WORD){
		cpu_ctx.RegDst_MUX = true;
		cpu_ctx.ALUSrc_MUX = false;
		cpu_ctx.MemtoReg_MUX = false;
	}

}

uint32_t MULTIPLEXOR(bool selector, uint32_t HIGH_INPUT, uint32_t LOW_INPUT){
    // Function takes in 2 unsigned 32 bit value, if the selector is True we return the HIGH_INPUT
    // else we return the LOW_INPUT because the selector is definitely false.
    uint32_t result_val;
    if (selector){                  //
        result_val = HIGH_INPUT;
    }
    else{
        result_val = LOW_INPUT;
    }

    return result_val;
}
