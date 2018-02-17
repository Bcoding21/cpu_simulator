/**
 * Gedare Bloom
 * Temilola Oloyede
 * Daniel Erhabor
 * David Awogbemila
 * Brandon Cole
 * single-cycle.c
 * Drives the simulation of a single-cycle processor
 */

#include "cpu.h"
#include "memory.h"
#include "syscall.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define DEBUG

int main( int argc, char *argv[] )
{
	FILE *f;
	struct IF_ID_buffer *if_id = (struct IF_ID_buffer*) malloc(sizeof(struct IF_ID_buffer*));
	struct ID_EX_buffer id_ex;
	struct EX_MEM_buffer ex_mem;
	struct MEM_WB_buffer mem_wb;
	int i;
	cpu_ctx.PC = 0x00400000;

	/* Initialize registers and memory to 0 */
	// cpu_ctx.PC = 0;
	for ( i = 0; i < 32; i++ ) {
		cpu_ctx.GPR[i] = 0;
	}

	for ( i = 0; i < 1024; i++ ) {
		instruction_memory[i] = 0;
		data_memory[i] = 0;
		stack_memory[i] = 0;
	}

	/* Read memory from the input file */
	f = fopen(argv[1], "r");
	if (!f){
		printf("File not found");
	}
	assert (f);
	for ( i = 0; i < 1024; i++ ) {
		// fread(&data_memory[i], sizeof(uint32_t), 1, f);
#if defined(DEBUG)
		// printf("%u\n", data_memory[i]);
#endif
	}
	for ( i = 0; i < 36; i++ ) {
		fread(&instruction_memory[i], sizeof(uint32_t), 1, f);
#if defined(DEBUG)
		printf("%u\n", instruction_memory[i]);
#endif
	}
	fclose(f);
	int count = 0;

	while(count < 36) {
// #if defined(DEBUG)
// 		printf("FETCH from PC=%x\n", cpu_ctx.PC);
// #endif
		fetch( if_id );
		// decode( &if_id, &id_ex );
		// execute( &id_ex, &ex_mem );
		// memory( &ex_mem, &mem_wb );
		// writeback( &mem_wb );
		if ( cpu_ctx.PC == 0 ) break;
		count++;
	}

	return 0;
}
