#ifndef _LIB_H
#define _LIB_H

#include <stdint.h>

void *memset(void * destination, int32_t character, uint64_t length);
void *memcpy(void * destination, const void * source, uint64_t length);
char *cpuVendor(char *result);
unsigned int strlenght(const char* str);
char *uint64ToString(uint64_t value, char *buffer, int base);

extern void refresh_registers();
extern uint64_t *get_registers();

#endif