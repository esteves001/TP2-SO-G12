#ifndef PROCESS_H_
#define PROCESS_H_

#include <stdint.h>
#include <stddef.h>

#define MAX_PROCESSES 16

typedef enum { READY = 0, RUNNING, BLOCKED, KILLED } process_state;

typedef struct {
    uint64_t pid;       // Pid del proceso
    uint64_t rsp;       // Puntero al stack para el scheduler
    char * process_name;      // Nombre del proceso
    int state;      // Estado del proceso(blocked, running, ready, killed es un estado inventado para saber cuando se debe retirar un proceso)
    //int quantum;          // ticks que le quedan
    //int base_quantum;     // quantum original para resetear
} pcb_t;

extern pcb_t* current_process; // Puntero al proceso actual corriendo
extern pcb_t* process_table[MAX_PROCESSES]; // Por lo pronto tenemos el array estatico con max 16 procesos y ordenados por pid (pid = 1 -> process_table[0], ..)

uint64_t sys_get_pid();
void scheduler();
void create_process(void * entry_point, const char * process_name);  // el entry_point es la direccion de memoria donde comienza el proceso
void exit_process();    // Termina el proceso actual 
void block_process(uint64_t pid);
void unblock_process(uint64_t pid);

// Funciones utiles segun claude
pcb_t* get_process(uint64_t pid); // Buscar proceso por pid, util internamente
uint64_t get_pid_count();         // Cuantos procesos hay corriendo, util para debug

#endif  