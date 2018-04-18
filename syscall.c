/**
 * Gedare Bloom
 * Temilola Oloyede
 * Daniel Erhabor
 * David Awogbemila
 * Brandon Cole
 * syscall.c
 * Implementation of the system calls
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "memory.h"

int sys_exit() {
	printf("Exiting\n");
	exit(0);
}

int syscall(int num, ...) {
	printf("syscall.c\n\n");
	va_list valist;
	va_start(valist, num);
	va_end(valist);
	int call = va_arg(valist, int);
	printf("syscall.c/call: %d\n", call);
	if (call == 10) {
			sys_exit();
			printf("syscall.c/exit\n"); //TODO: remove debug statement
	} else if (call == 1) {
		// Print int
		int print_arg = va_arg(valist, int);
		printf("print_arg: %d\n", print_arg);
		printf("syscall.c/Print int!!!!\n"); //TODO: remove debug statement
		printf("%d", print_arg);
	} else if (call == 4) {
		// print string
		int string_data_index = va_arg(valist, int);
		printf("string_data_index: %d\n", string_data_index);
		printf("syscall.c/Print string!!!!\n"); //TODO: remove debug statement
		//char string[] = {};
		//memcpy(string, data_memory[string_data_index], sizeof(data_memory[string_data_index]));
		//printf("%s", string);
	}
}
