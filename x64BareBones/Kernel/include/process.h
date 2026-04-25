#ifndef PROCESS_H_
#define PROCESS_H_

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t pid;       // Pid del proceso
    uint64_t rsp;       // Puntero al stack para el scheduler
    char process_name[32];      // Nombre del proceso
    int state;      // Estado del proceso(blocked, runningm, ready)
} pcb_t;

extern pcb_t* current_process; // Puntero al proceso actual corriendo

uint64_t sys_get_pid();

#endif  