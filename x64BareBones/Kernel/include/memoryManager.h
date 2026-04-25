#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096 // 4KB por página

// Inicializa el Memory Manager marcando los bloques ocupados por el OS
void initializeMemoryManager(void);

// Reserva una página de 4 KiB y devuelve el puntero al inicio de la misma
void* allocatePage(void);

// Libera una página previamente asignada
void freePage(void* address);

#endif