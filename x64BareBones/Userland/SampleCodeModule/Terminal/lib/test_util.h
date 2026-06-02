#ifndef _TEST_UTIL_H_
#define _TEST_UTIL_H_

#include <stdint.h>

// PRNG simple (el clasico de la catedra) para pedir tamaños/valores random
uint32_t GetUint(void);
uint32_t GetUniform(uint32_t max);

// verifica que los 'size' bytes desde 'start' valgan todos 'value'
uint8_t memcheck(void * start, uint8_t value, uint32_t size);

// string -> int (para leer el parametro de memoria maxima)
int64_t satoi(char * str);

#endif
