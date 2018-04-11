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

void showRegisterValues();
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
	for ( i = 0; i < 32; i++ ) {
		cpu_ctx.GPR[i] = 0;
		l1_data_cache[32].fill_extent = 0;
	}

	for ( i = 0; i < 1024; i++ ) {
		instruction_memory[i] = 0;
		data_memory[i] = 0;
		stack_memory[i] = 0;
	}

	const char* file = "example-text.txt";
	/* Read memory from the input file */
	f = fopen(file, "r");
	if (!f){
		printf("File not found");
	}
	//assert (f);
	for ( i = 0; i < 14; i++ ) {		//	only 12 instructions are read in because the programs we use to test only have 4 instruction. We'll switch to 1024 finally.
		fread(instruction_memory + i, sizeof(uint32_t), 1, f);
#if defined(DEBUG)
		printf("i:%x\n", instruction_memory[i]);
#endif
	}

	/*for ( i = 0; i < 4; i++ ) {		//	only 4 words of data are read in because the programs we use to test only have 4 words of data. We'll switch to 1024 finally.
		fread(&data_memory[i], sizeof(uint32_t), 1, f);
#if defined(DEBUG)
		printf("d:%x\n", data_memory[i]);
#endif
	}*/
	fclose(f);
	int count = 0;

	while(1) {
#if defined(DEBUG)
		printf("FETCH from PC=%x\n", cpu_ctx.PC);
#endif
		printf("press Enter to run next cycle.\n");
		char c[2];
		fgets(c, sizeof c, stdin);

		fetch( if_id );
		decode( if_id, &id_ex );
		execute( &id_ex, &ex_mem );
		memory( &ex_mem, &mem_wb );
		writeback( &mem_wb );
		showRegisterValues(cpu_ctx.GPR);
		if ( cpu_ctx.PC == 0 ) {
			break;
		}
		printf("%i\n", count);
		count++;
	}

	return 0;
}
// This function displays the values in the GPR array. We use this for debugging purposes really.
void showRegisterValues(int gpr[]) {
	printf("GPR: [ ");
	for(int i = 0; i < 32; i++) {
		printf ("%d ", i);
	}
	printf("]\n");
	printf("GPR: [ ");
	for(int i = 0; i < 32; i++) {
		printf ("%d ", gpr[i]);
	}
	printf("]\n");
}
