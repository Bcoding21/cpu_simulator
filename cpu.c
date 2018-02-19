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
int setControlSignals(short opcode, short funct);

struct cpu_context cpu_ctx;

int fetch(struct IF_ID_buffer *out )
{
	instructionMemory(cpu_ctx.PC, out);
	out->pc_plus_4 = cpu_ctx.PC + 1;
	return 0;
}

int decode( struct IF_ID_buffer *in, struct ID_EX_buffer *out )
{
	short opcode = in->instruction >> 26; // gets bits 31:26
	short funct = in->instruction & 0x3F; // gets bits 5:0

	setControlSignals(opcode, funct); // set the control signals

    uint32_t RS_index = (in->instruction >> 21) & 0x1F; // get bits 25:21 (rs)
    uint32_t RT_index = (in->instruction >> 16) & 0x1F; // get bits 20:16(rt)
    uint32_t RD_index = (in->instruction >> 11) & 0x1F; // get bits 15:11(rd)
    //	get jump target address
    uint32_t jump_target_address = in->instruction & 0x3FFFFFF;
    jump_target_address = jump_target_address << 2;
    //	get pc_plus_4 leftmost 4 bits
    jump_target_address = (in->pc_plus_4 & 0xF0000000 ) | jump_target_address;

    // set data outputs
	out->read_data_1 = cpu_ctx.GPR[RS_index];     // RegisterFile[rs]
    out->read_data_2 = cpu_ctx.GPR[RT_index];     // RegisterFile[rt]
    //              if MSB(immediate field) == 0, then sign extend with leading 0s else sign extend with leading 1s
    out->immediate = (in->instruction & 0x8000) == 0 ? in->instruction & 0xFFFF : in->instruction & 0xFFFF0000; // 15:0  address
 	out->funct = funct; // bits 5:0 funct
    out->pc_plus_4 = in->pc_plus_4;   //FIX ME: Correct naming later

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
    out->jump_register = cpu_ctx.CNTRL.jump_register;
    out->jump_target_address = jump_target_address;

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

    struct ALU_INPUT* alu_input = (struct ALU_INPUT*) malloc(sizeof(struct ALU_INPUT)); // holds inputs
    struct ALU_OUTPUT* alu_output = (struct ALU_OUTPUT*) malloc(sizeof(struct ALU_OUTPUT)); // hold outputs

    // inputs
    //	use MUX to pick PC + 4 and 0 for jal instruction to be stored in $ra ($31).
    alu_input->input_1 = MULTIPLEXOR(in->jump, in->pc_plus_4, in->read_data_1);
    uint32_t alu_src_mux_output = MULTIPLEXOR(in->alu_source, in->immediate, in->read_data_2);
    alu_input->input_2 = MULTIPLEXOR(in->jump, 0, alu_src_mux_output);

	alu_input->funct = in->funct;
	alu_input->opcode = in->opcode;
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
    out->jump_register = in->jump_register;
    out->jump_target_address = in->jump_target_address;

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
	bool pc_src = in->branch && in->branch_result;
	uint32_t pc_src_mux_output = MULTIPLEXOR(pc_src, in->branch_target, in->pc_plus_4);
	uint32_t jump_mux_output = MULTIPLEXOR(in->jump, in->jump_target_address, pc_src_mux_output);
	cpu_ctx.PC  = MULTIPLEXOR (in->jump_register, cpu_ctx.GPR[31], jump_mux_output);

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

int setControlSignals(short opcode, short funct) { // sets control signal outputs
	// R type instruction
	if(opcode == 0x0) {
		if (funct == 0x8) {	// jr $ra should not write to regiser file
			cpu_ctx.CNTRL.reg_write = false;
			cpu_ctx.CNTRL.jump_register = true;
		}
		else {
			cpu_ctx.CNTRL.reg_write = true;
			cpu_ctx.CNTRL.jump_register = false;
		}	
		//False signals
		cpu_ctx.CNTRL.mem_read = false;
		cpu_ctx.CNTRL.mem_write = false;
		cpu_ctx.CNTRL.branch = false;
		cpu_ctx.CNTRL.mem_to_reg = false;
		cpu_ctx.CNTRL.alu_source = false;
		cpu_ctx.CNTRL.jump = false;
		//True signals
		cpu_ctx.CNTRL.reg_dst = true;
	} 
	//				   j   			   jal
	else if(opcode == 0x2 || opcode == 0x3) {
		if (funct == 0x2) {	// j should not write to regiser file
			cpu_ctx.CNTRL.reg_write = false;
		}				  
		else if (funct == 0x3) {//jal should write to the register file ($ra, $31)
			cpu_ctx.CNTRL.reg_write = true;
		}	
		//	False signals
		cpu_ctx.CNTRL.mem_read = false;
		cpu_ctx.CNTRL.mem_write = false;
		cpu_ctx.CNTRL.branch = false;
		cpu_ctx.CNTRL.mem_to_reg = false;
		cpu_ctx.CNTRL.alu_source = false;	// really a don't care
		cpu_ctx.CNTRL.reg_dst = false;		// really a don't care

		//True signals
		cpu_ctx.CNTRL.jump = true;
	}
	// I type instructions
	else {	// see table
		cpu_ctx.CNTRL.reg_write = true;
		cpu_ctx.CNTRL.mem_read = false;
		cpu_ctx.CNTRL.mem_write = false;
		cpu_ctx.CNTRL.branch = false;
		cpu_ctx.CNTRL.mem_to_reg = false;
		cpu_ctx.CNTRL.alu_source = true;
		cpu_ctx.CNTRL.jump = false;
		cpu_ctx.CNTRL.reg_dst = false;
		cpu_ctx.CNTRL.jump_register = false;
					//bne              beq
		if (opcode == 0x5 || opcode == 0x4) {
			cpu_ctx.CNTRL.reg_write = false;
			cpu_ctx.CNTRL.branch= true;
			// reg_dst for beq and bne are falsely dont care
		}
					//		lw
		else if ( opcode == 0x23 ) {
			cpu_ctx.CNTRL.reg_write = true;
			cpu_ctx.CNTRL.mem_read = true;
			cpu_ctx.CNTRL.mem_write = false;
			cpu_ctx.CNTRL.mem_to_reg = true;
		} 			//		sw
		else if (opcode == 0x2B) {
			cpu_ctx.CNTRL.reg_write = false;
			cpu_ctx.CNTRL.mem_read = false; // dont care
			cpu_ctx.CNTRL.mem_write = true;
			cpu_ctx.CNTRL.mem_to_reg = false; //dont care
		} 
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
