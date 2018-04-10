/**
 * Gedare Bloom
 * Temilola Oloyede
 * Daniel Erhabor
 * David Awogbemila
 * Brandon Cole.
 * cpu.c
 * Implementation of simulated processor.
 */

#define NUM_INDEX_BITS 5
#define NUM_OFFSET_BITS 4
#define NUM_SETS 32
#define BLOCKS_PER_SET 4
#define BLOCK_SIZE 4 // words
#define SET_SIZE 4
#define BYTES_PER_WORD 4
#define L1_DATA_START_ADDRESS 0x10000000

#include "cpu.h"
#include "memory.h"
#include <stdlib.h>
#include <stdio.h>
#include "syscall.h"

int instructionMemory(uint32_t address, struct IF_ID_buffer *out);
int registerFile(struct REG_FILE_input* input, struct REG_FILE_output* output);
int alu(struct ALU_INPUT* alu_input, struct ALU_OUTPUT* out);
int setControlSignals(short opcode, short funct);
uint32_t readWordFromDataCache(uint32_t addr);
void writeDataCache(uint32_t, uint32_t);

struct cpu_context cpu_ctx;
struct Set l1_data_cache[32];

int fetch(struct IF_ID_buffer *out )
{
	instructionMemory(cpu_ctx.PC, out);
	out->pc_plus_4 = cpu_ctx.PC + 4;
	return 0;
}

int decode( struct IF_ID_buffer *in, struct ID_EX_buffer *out )
{
	short opcode = in->instruction >> 26; // gets bits 31:26
	short funct = in->instruction & 0x3F; // gets bits 5:0

	// TODO: move to setControlSignals
	if (opcode == 0 && funct == 0xc && ((in->instruction & 0x3FFFFC0) == 0)) {  // check if it's a syscall
		cpu_ctx.interrupt= true;
	} else {
		cpu_ctx.interrupt = false;
	}

	uint32_t shamt = (in->instruction >> 6) & 0x1F; // gets bits 10:6

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
	out->read_data_1 = cpu_ctx.GPR[opcode == 0x0 && funct == 0x0 ? RT_index: RS_index];     // RegisterFile[rs]
    out->read_data_2 = cpu_ctx.GPR[RT_index];     // RegisterFile[rt]
    //              if MSB(immediate field) == 0, then sign extend with leading 0s else sign extend with leading 1s
    out->immediate = (in->instruction & 0x8000) == 0 ? in->instruction & 0xFFFF : in->instruction & 0xFFFF0000; // 15:0  address
 	out->funct = funct; // bits 5:0 funct

 	out->shamt = shamt;
 	out->pc_plus_4 = in->pc_plus_4;

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
    out->jump = cpu_ctx.CNTRL.jump;
    out->reg_write = cpu_ctx.CNTRL.reg_write;
    out->jump_register = cpu_ctx.CNTRL.jump_register;
    out->jump_target_address = jump_target_address;
    out->interrupt = cpu_ctx.interrupt;
    out->opcode = opcode;
    return 0;
}

int execute( struct ID_EX_buffer *in, struct EX_MEM_buffer *out )
{
    out->write_reg_index = MULTIPLEXOR(in->reg_dst, in->RD_index, in->RT_index); // Rg_Dst MUX with its inputs and selector
		// interrupt
    if (in->interrupt) {
    	syscall(2, cpu_ctx.GPR[2], cpu_ctx.GPR[4]);
    	out->pc_plus_4 = in->pc_plus_4;
    	return 0;
    }


    struct ALU_INPUT* alu_input = (struct ALU_INPUT*) malloc(sizeof(struct ALU_INPUT)); // holds inputs
    struct ALU_OUTPUT* alu_output = (struct ALU_OUTPUT*) malloc(sizeof(struct ALU_OUTPUT)); // hold outputs

    // inputs
    //	use MUX to pick PC + 4 and 0 for jal instruction to be stored in $ra ($31).
    alu_input->input_1 = MULTIPLEXOR(in->jump, in->pc_plus_4, in->read_data_1);
    uint32_t alu_src_mux_output = MULTIPLEXOR(in->alu_source, in->immediate, in->read_data_2);
    alu_input->input_2 = MULTIPLEXOR(in->jump, 0, alu_src_mux_output);

    // alu_input will take funct, opcode and shamt
    alu_input->funct = in->funct;
    alu_input->opcode = in->opcode;
    alu_input->shamt = in->shamt;
    alu(alu_input, alu_output);

    // pass alu results
    out->alu_result = alu_output->alu_result;
    out->branch_result = alu_output->branch_result;
    out->read_data_2 = in->read_data_2;
    // pass control signals to next buffer
    out->branch = in->branch;
    out->mem_read = in->mem_read;
    out->reg_write = in->reg_write;
    out->mem_write = in->mem_write;
    out->mem_to_reg = in->mem_to_reg;
    out->pc_plus_4 = in->pc_plus_4;
    // I subtract here because I have reason to believe qtspim calculates offset from pc + 4 and not from pc
    // subject to the label being ona line of its own
    out->branch_target = ((in->immediate - 1) << 2) + in->pc_plus_4;
    out->jump_register = in->jump_register;
    out->jump_target_address = in->jump_target_address;
    out->interrupt = cpu_ctx.interrupt;

    return 0;
}

int memory( struct EX_MEM_buffer *in, struct MEM_WB_buffer *out )
{
	if (in->interrupt) {
		return 0;
	}
	uint32_t write_address = in->alu_result;
	uint32_t write_data = in->read_data_2;
	if (in->mem_write) {
		writeDataCache(write_address, write_data);
	}

	//fake implementation of PCSrc signal and MUX
	bool pc_src = in->branch && in->branch_result;
	uint32_t pc_src_mux_output = MULTIPLEXOR(pc_src, in->branch_target, in->pc_plus_4);
	uint32_t jump_mux_output = MULTIPLEXOR(in->jump, in->jump_target_address, pc_src_mux_output);
	cpu_ctx.PC  = MULTIPLEXOR (in->jump_register, cpu_ctx.GPR[31], jump_mux_output);

	//	pass necessary information to MEM/WB buffer
	out->reg_write = in->reg_write;
	if (in->mem_read) {
		out->mem_read_data = data_memory[write_address - 0x10000000];
		#if defined(ENABLE_L1_CACHES)
		printf("Using cache for memory operation.\n");
		out->mem_read_data = readWordFromDataCache(write_address);
		#endif
	}
	out->mem_to_reg = in->mem_to_reg;
	out->alu_result = in->alu_result;
	out->write_reg_index = in->write_reg_index;
	out->interrupt = cpu_ctx.interrupt;
	out->jump = in->jump;
	return 0;
}

int writeback( struct MEM_WB_buffer *in ){

	if (in->interrupt) {
		return 0;
	}
	in->write_reg_index = MULTIPLEXOR(in->jump, 31, in->write_reg_index);
	if(in->reg_write) {
		cpu_ctx.GPR[in->write_reg_index] = MULTIPLEXOR(in->mem_to_reg, in->mem_read_data, in->alu_result);
	}
	return 0;
}

// this function returns a block referred to by addr in the 
// det associative data memory cache is 
// parameter addr is the address of the WORD (NOT BLOCK requested)
// it returns on ly the word at the requested address (addr) not the entire block

uint32_t readWordFromDataCache(uint32_t addr) {
	uint32_t block_addr = addr >> 4;		// block address = memory address / size of block (16)
	uint32_t block_tag = addr >> 9;			// get most significant 25 bits of address for tag
	int setIndex = block_addr % 32;			// get index for set
	struct Set requiredSet = l1_data_cache[setIndex];	//obtain required set
	int word_offset = (addr - (block_addr << 4)) / 4;	//get byte offset for particular word
	struct Block required_block;

	bool found_and_valid = false;
	printf("Block tag: %x\n", block_tag);
	for (int i = 0; i < 4; i++) {
		printf("tag %d: %x\n", i, requiredSet.block_array[i].tag);
		if(requiredSet.block_array[i].tag == block_tag && requiredSet.block_array[i].valid) {
			printf("a hit! a hit! we have a hit! getting block from cache and not memory.\n");
			//handle read hit
			required_block = requiredSet.block_array[i];
			found_and_valid = true;

			// update lru_states (only states less than former state of new mru need to be increased)
			int former_lru_state = requiredSet.lru_states[i]; // this is the formaer LRU state of the block that's being read
			l1_data_cache[setIndex].lru_states[i] = 0;
			for (int j = 0; j < requiredSet.fill_extent; j++){
				if (i != j && requiredSet.lru_states[j] < former_lru_state) { requiredSet.lru_states[j]++;		}
			}
		}
	}

	//handle read miss
	//need to implement logic to: (lol not done in specified order)
	//1. get block from memory
	//2. decide which block to evict
	//3. replace blocks that needs to be evicted
	//4. update lru state
	if(!found_and_valid) {
		printf("we have a miss.\n");
		//only do eviction if we have four blocks
		if(requiredSet.fill_extent == 4 ) {
			printf("Conflict miss.\n");
			cpu_ctx.stall_count += 4; //need to increase stall count
			int index_of_lru_block = 0;
			for (int i = 0; i < 4; i++) {
				//get the index of the block with the highest LRU state which 
				index_of_lru_block = requiredSet.lru_states[i] > requiredSet.lru_states[index_of_lru_block] ? i : index_of_lru_block;
			}
			int raw_data_mem_array_index = ((block_addr << 4) - 0x10000000) / 4;
			//create new block form data memory
			required_block.data[0] = data_memory[raw_data_mem_array_index];
			required_block.data[1] = data_memory[raw_data_mem_array_index + 1];
			required_block.data[2] = data_memory[raw_data_mem_array_index + 2];
			required_block.data[3] = data_memory[raw_data_mem_array_index + 3];
			required_block.valid = true;
			required_block.tag = block_tag;

			//lru block will be replaced and so will become the MRU, so it'll have a state of 0
			l1_data_cache[setIndex].lru_states[index_of_lru_block] = 0;
			l1_data_cache[setIndex].block_array[index_of_lru_block] = required_block;
			for (int i = 0; i < 4; i++) {
				if (i != index_of_lru_block) { requiredSet.lru_states[i]++; }
			}
		} else {
			printf("Compulsory miss.\n");
			//handle compulsory misses (when miss occurs because set is not full)
			int index_of_lru_block = 0;
			for (int i = 0; i < requiredSet.fill_extent; i++) {
				//get the index of the block with the highest LRU state which 
				index_of_lru_block = requiredSet.lru_states[i] > requiredSet.lru_states[index_of_lru_block] ? i : index_of_lru_block;
			}
			requiredSet.fill_extent += 1;
			int raw_data_mem_array_index = ((block_addr << 4) - 0x10000000) / 4;
			//create new block form data memory
			required_block.data[0] = data_memory[raw_data_mem_array_index];
			required_block.data[1] = data_memory[raw_data_mem_array_index + 1];
			required_block.data[2] = data_memory[raw_data_mem_array_index + 2];
			required_block.data[3] = data_memory[raw_data_mem_array_index + 3];
			required_block.valid = true;
			required_block.tag = block_tag;
			l1_data_cache[setIndex].block_array[requiredSet.fill_extent - 1] = required_block;
			for (int i = 0; i < requiredSet.fill_extent; i++) {
				if (i != index_of_lru_block) { requiredSet.lru_states[i]++; }
			}	
		}
	}
	return required_block.data[word_offset];
}

int instructionMemory(uint32_t address, struct IF_ID_buffer *out) {
	out->instruction = instruction_memory[(address - 0x400000) / 4];
	// printf("Instruction got: %d\n", out->instruction);
	return 0;
}

int alu(struct ALU_INPUT* alu_input, struct ALU_OUTPUT* out){
	printf("opcode: %d\n", alu_input->opcode);
	if (alu_input->opcode == 0x00) { // R-format and syscall
		if (alu_input->funct == 0x20) { out->alu_result = alu_input->input_1 + alu_input->input_2; }				//add
		else if(alu_input->funct == 0x24)  { out->alu_result = alu_input->input_1 & alu_input->input_2; }			//and
		else if(alu_input->funct == 0x27)  { out->alu_result = ~(alu_input->input_1 | alu_input->input_2); }		//nor
		else if(alu_input->funct == 0x25)  { out->alu_result = alu_input->input_1 | alu_input->input_2; }			//or
		else if(alu_input->funct == 0x2A)  { out->alu_result = (alu_input->input_1 < alu_input->input_2) ? 1 : 0; }	//slt
		else if (alu_input->funct == 0x00) {out->alu_result = alu_input->input_1 << alu_input->shamt;}              // sll
		else if (alu_input->funct == 0x02) {out->alu_result = alu_input->input_1 >> alu_input->shamt;}              // srl
        else if(alu_input->funct == 0x22)  { out->alu_result = alu_input->input_1 - alu_input->input_2; }           // sub

        // Input casted to signed then shifted, then result is casted to unsigned to keep with out->alu_result type of uint32_t
		else if (alu_input->funct == 0x03) {out->alu_result = (uint32_t) ((int32_t)alu_input->input_1 >> alu_input->shamt);} // sra
		else if(alu_input->funct == 0x26)  { 
			out->alu_result = alu_input->input_1 ^ alu_input->input_2;												// xor
		}   
		// No operation need for jr

		// FIX ME: Conditions for syscall maybe

	}
	//                  J                              Jal
	else if(alu_input->opcode == 0x02 || alu_input->opcode == 0x03 ) {
		// No need to do anything for J just add both alu inputs for Jal. Specifications handled in the Jal MUX area
		if (alu_input->opcode == 0x03 ) {out->alu_result = alu_input->input_1 + alu_input->input_2;}                  // Jal
	}
	// I format instructions.
	else {
        if (alu_input->opcode == 0x08 ) {out->alu_result = alu_input->input_1 + alu_input->input_2; }                        // addi
        else if (alu_input->opcode == 0x0C ) {out->alu_result = alu_input->input_1 & alu_input->input_2;}                   // andi
        else if (alu_input->opcode == 0x0F ) {out->alu_result = (alu_input->input_2 << 16);}                                // lui
        else if (alu_input->opcode == 0x0D ) {out->alu_result = alu_input->input_1 | alu_input->input_2;}                   // ori
        else if (alu_input->opcode == 0x0A )  { out->alu_result = (alu_input->input_1 < alu_input->input_2) ? 1 : 0; }	    // slti
        else if(alu_input->opcode == 0x0E)  { out->alu_result = alu_input->input_1 ^ alu_input->input_2; }                  // xori

        else if (alu_input->opcode == 0x04) {
        	out->branch_result = (alu_input->input_1 == alu_input->input_2) ? 1 : 0 ;
        }     // beq
        else if (alu_input->opcode == 0x05) {out->branch_result = (alu_input->input_1 != alu_input->input_2) ? 1 : 0 ;}     // bne

        else if (alu_input->opcode == 0x23 ) {out->alu_result = alu_input->input_1 + alu_input->input_2;}                   // lw
        else if (alu_input->opcode == 0x2B ) {out->alu_result = alu_input->input_1 + alu_input->input_2;}                   // sw

    }

    return 0;
}

int setControlSignals(short opcode, short funct) { // sets control signal outputs
	// R type instruction
	if(opcode == 0x00) {
		if (funct == 0x08) {	// jr $ra should not write to regiser file
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
	else if(opcode == 0x02 || opcode == 0x03) {
		if (funct == 0x02) {	// j should not write to regiser file
			cpu_ctx.CNTRL.reg_write = false;
		}
		else if (funct == 0x03) {//jal should write to the register file ($ra, $31)
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
		if (opcode == 0x05 || opcode == 0x04) {
			cpu_ctx.CNTRL.reg_write = false;
			cpu_ctx.CNTRL.branch= true;
			cpu_ctx.CNTRL.alu_source = false;
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

	return 0;
}

uint32_t MULTIPLEXOR(bool selector, uint32_t HIGH_INPUT, uint32_t LOW_INPUT){
    // Function takes in 2 unsigned 32 bit value, if the selector is True we return the HIGH_INPUT
    // else we return the LOW_INPUT because the selector is definitely false.
	uint32_t result_val;
    if (selector){                  
    	result_val = HIGH_INPUT;
    }
    else{
    	result_val = LOW_INPUT;
    }

    return result_val;
}

/*read block into cache*/
void readMem(struct Block* block, uint32_t address, uint32_t* memory) {
	uint32_t pos = (address - L1_DATA_START_ADDRESS) / BLOCK_SIZE;
	for (int i = 0; i < BLOCK_SIZE; i++) {
		block->data[i] = memory[pos + i];
	}
	block->offset = address & ((1 << NUM_OFFSET_BITS) - 1);
	block->index = address % 64;
	block->tag = address >> (NUM_OFFSET_BITS + NUM_INDEX_BITS);
	block->valid = true;
}

/*write block to memory*/
void writeMem(struct Block* block, uint32_t address) {
	uint32_t pos = (address - L1_DATA_START_ADDRESS) / BLOCK_SIZE;
	for (int i = 0; i < BLOCK_SIZE; i++) {
		data_memory[pos + i] = block->data[i];
	}
}

/*Write miss policy*/
void writeAllocate(struct Set* set, uint32_t address, uint32_t data) {
	/*determine block to be replaced*/
	int greatest = set->lru_states[0];
	int blockPos = 0;
	for (int i = 0; i < SET_SIZE; i++) {
		if (set->lru_states[i] > greatest) {
			blockPos = i;
		}
	}

	/* save data in memory if dirty.*/
	struct Block* block = set->block_array + blockPos;

	if (block->dirty) {
		writeMem(block, address);
	}

	/*Then replace block in cache with block from mem.*/
	readMem(block, address, data_memory); 
	uint32_t dataPos = block->offset >> BLOCK_SIZE; // div by 4
	block->data[dataPos] = data;

	/*updates lru states*/
	for (int i = 0; i < SET_SIZE; i++) {set->lru_states[i]++;}
	set->lru_states[blockPos] = 0;
}

/*Wirte hit policy*/
void writeBack(struct Set* set, uint32_t blockPos, uint32_t data) {

	struct Block* block = set->block_array + blockPos;
	/*write data*/
	uint32_t dataPos = block->offset >> BLOCK_SIZE; // div by 4
	block->data[dataPos] = data;
	block->dirty = true;

	/*update lru states*/
	int* currBlockState = set->lru_states + blockPos; 
	for (int i = 0; i < SET_SIZE; i++) { 
		int* otherBlockState = set->lru_states + i;
		*otherBlockState += (*currBlockState < *otherBlockState);
	}
	*currBlockState = 0;
}

/*Returns -1 if a miss occurs*/
int getBlockPos(struct Set* set, uint32_t tag) {
	for (int i = 0; i < BLOCKS_PER_SET; i++) { // find block in set
		struct Block* currBlock = set->block_array + i;
		if (currBlock->valid && currBlock->tag == tag) {
			return i;
		}
	}
	return -1;
}

/*Writes data to cache using write back policy for hits
and write allocate for misses*/
void writeDataCache(uint32_t address, uint32_t data) {

	uint32_t blockAddress = address >> BLOCK_SIZE;
	uint32_t setIndex = blockAddress % NUM_SETS;
	struct Set* set = l1_data_cache + setIndex;

	uint32_t tag = address >> (NUM_OFFSET_BITS + NUM_INDEX_BITS);

	int blockPos = getBlockPos(set, tag);

	if (blockPos != -1) {
		writeBack(set, blockPos, data);
	}
	else {
		writeAllocate(set, address, data);
	}
}
