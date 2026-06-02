#ifndef _MEMLIB_H_
#define _MEMLIB_H_

#include <stdint.h>

// malloc/free de userland: le pido/devuelvo memoria al kernel por syscall
void * malloc(uint64_t size);
void   free(void * ptr);

// memset ya esta definido en _loader.c (se usa al boot p/ limpiar el bss).
// aca solo lo declaro asi lo puedo usar desde los tests sin redefinirlo.
void * memset(void * dest, int32_t c, uint64_t n);

#endif
