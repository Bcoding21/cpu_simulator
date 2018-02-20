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
#include "syscall.h"
int instructionMemory(uint32_t address, struct IF_ID_buffer *out);
int registerFile(struct REG_FILE_input* input, struct REG_FILE_output* output);
int alu(struct ALU_INPUT* alu_input, struct ALU_OUTPUT* out);

struct cpu_context cpu_ctx;

int fetch(struct IF_ID_buffer *out )
{
	instructionMemory(cpu_ctx.PC, out);
	out->next_pc = cpu_ctx.PC + 1;
	return 0;
}

int decode( struct IF_ID_buffer *in, struct ID_EX_buffer *out )
{
	printf("decode\n");
	short opcode = in->instruction >> 26; // gets bits 31:26
	short funct = in->instruction & 0x3F; // gets bits 5:0

	//setControlSignals(opcode, funct); // set the control signals

	// TODO: move to setControlSignals
	if (opcode == 0 && funct == 0xc && ((in->instruction & 0x3FFFFC0) == 0)) {  // check if it's a syscall
		printf("decode/syscall\n");
		cpu_ctx.interrupt= true;
	}

    uint32_t RS_index = (in->instruction >> 21) & 0x1F; // get bits 25:21 (rs)
    uint32_t RT_index = (in->instruction >> 16) & 0x1F; // get bits 20:16(rt)

    uint32_t RD_index = (in->instruction >> 11) & 0x1F; // get bits 15:11(rd)



    // set data outputs
	out->read_data_1 = cpu_ctx.GPR[RS_index];     // RegisterFile[rs]
    out->read_data_2 = cpu_ctx.GPR[RT_index];     // RegisterFile[rt]
    //              if MSB(immediate field) == 0, then sign extend with leading 0s else sign extend with leading 1s
    out->immediate = (in->instruction & 0x8000) == 0 ? in->instruction & 0xFFFF : in->instruction & 0xFFFF0000; // 15:0  address
 	out->funct = funct; // bits 5:0 funct
    out->pc_plus_4 = in->next_pc;   //FIX ME: Correct naming later

    out->RS_index = RS_index; // Not used yet. Will be used in conjunction with forwarding unit when pipelining for data hazards

    out->RT_index = RT_index;
    out->RD_index = RD_index;




    // pass signals to next buffer
    out->reg_dst = cpu_ctx.CNTRL.reg_dst;
    out->mem_write = cpu_ctx.CNTRL.mem_write;
    out->mem_read = cpu_ctx.CNTRL.mem_read;
    out->branch = cpu_ctx.CNTRL.branch;
    out->mem_to_reg = cpu_ctx.CNTRL.mem_to_reg;
    out->alu_source = cpu_ctx.CNTRL.alu_source;
    out->reg_write = cpu_ctx.CNTRL.reg_write;
		out->interrupt = cpu_ctx.interrupt;

    // debugging
	printf("opcode: %d\n", out->opcode);
	printf("read_data_1: %d\n", out->read_data_1);
	printf("read_data_2: %d\n", out->read_data_2);
	printf("funct: %d\n", out->funct);
	return 0;
}

int execute( struct ID_EX_buffer *in, struct EX_MEM_buffer *out )
{
    out->write_reg_index = MULTIPLEXOR(in->reg_dst, in->RD_index, in->RT_index); // Rg_Dst MUX with its inputs and selector
		printf("execute\n");
		// interrupt
		if (in->interrupt) {
			printf("execute/syscall\n");
			syscall(cpu_ctx.GPR[2], cpu_ctx.GPR[4]);
			out->pc_plus_4 = in->pc_plus_4;
			return 0;
		}


    struct ALU_INPUT* alu_input = (struct ALU_INPUT*) malloc(sizeof(struct ALU_INPUT)); // holds inputs

    // inputs
    alu_input->input_1 = in->read_data_1;
    alu_input->input_2 = (in->alu_source) ? in->immediate : in->read_data_2;
	alu_input->funct = in->funct;
	alu_input->opcode = in->opcode;
    struct ALU_OUTPUT* alu_output = (struct ALU_OUTPUT*) malloc(sizeof(struct ALU_OUTPUT)); // hold outputs
    alu(alu_input, alu_output);

    // pass alu results
    out->alu_result = alu_output->alu_result;
    out->branch_result = alu_output->branch_result;

    // pass control signals to next buffer
    out->branch = in->branch;
    out->mem_read = in->mem_read;
    out->reg_write = in->reg_write;
    out->mem_write = in->mem_write;
    out->mem_to_reg = in->mem_to_reg;
    out->pc_plus_4 = in->pc_plus_4;
    out->branch_target = (in->immediate << 2) + in->pc_plus_4;
	return 0;
}

int memory( struct EX_MEM_buffer *in, struct MEM_WB_buffer *out )
{
	uint32_t write_address = in->alu_result;
	bool branch = in->branch_result;
	uint32_t write_data = in->write_data;
	if (in->mem_write) {
		data_memory[write_address - 0x10000000] = write_data;
	}

	//fake implementation of PCSrc signal and MUX
	if (in->branch && in->branch_result) {
		cpu_ctx.PC = in->branch_target;
		printf("used branch!\n");
	} else {
		cpu_ctx.PC = in->pc_plus_4;
		printf("used not branch!\n");
	}

	//	pass necessary information to MEM/WB buffer
	out->reg_write = in->reg_write;
	if (in->mem_read) {
		out->mem_write_data = data_memory[write_address - 0x10000000];
	}
	out->mem_to_reg = in->mem_to_reg;
	out->alu_result = write_data;
	out->write_reg_index = in->write_reg_index;
	return 0;
}

int writeback( struct MEM_WB_buffer *in ){

    in->write_reg_index = MULTIPLEXOR(in->jump, 31, in->write_reg_index);
	if(in->reg_write) {
		//cpu_ctx.GPR[in->write_reg_index] = (in->mem_to_reg) ? in->mem_write_data : in->alu_result;
		cpu_ctx.GPR[in->write_reg_index] = MULTIPLEXOR(in->mem_to_reg, in->mem_write_data, in->alu_result);

	}
	return 0;
}

int instructionMemory(uint32_t address, struct IF_ID_buffer *out) {
	out->instruction = instruction_memory[address - 0x400000];
	// printf("Instruction got: %d\n", out->instruction);
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
			break;

		case LOAD_WORD:
			cpu_ctx.CNTRL.reg_write = true;
			cpu_ctx.CNTRL.mem_read = true;
			cpu_ctx.CNTRL.mem_write = false;
			cpu_ctx.CNTRL.branch = false;
			break;

		case STORE_WORD:
			cpu_ctx.CNTRL.reg_write = false;
			cpu_ctx.CNTRL.mem_read = false;
			cpu_ctx.CNTRL.mem_write = true;
			cpu_ctx.CNTRL.branch = false;
			break;

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
