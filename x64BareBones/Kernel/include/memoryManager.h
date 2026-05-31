#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096 // 4KB por página

// Inicializa el Memory Manager marcando los bloques ocupados por el OS
void initialize_memory_manager(void);

// Reserva una página de 4 KiB y devuelve el puntero al inicio de la misma
void* allocate_page(); // para poder testear pasa a btyes

// Libera una página previamente asignada
void free_page(void* address);

// Lo que devuelvo en la syscall mem_stats. Total/used/free en bytes.
// IMPORTANTE: definicion duplicada en syscallLib.h (userland).
// Si toco campos aca tambien hay que tocarlos alla (TODO: header compartido).
typedef struct {
    uint64_t total_bytes;   // memoria administrable, sin contar lo que se come el kernel ni el bitmap
    uint64_t used_bytes;    // lo alocado hasta ahora
    uint64_t free_bytes;    // lo que queda libre
} mem_info_t;

// Llena out con el estado actual del manager. La usa la syscall 0x29.
void get_mem_stats(mem_info_t * out);


#endif