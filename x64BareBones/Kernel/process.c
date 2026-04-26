#include "process.h"
#include "memoryManager.h"
#include "lib.h"

static int last_run_index = 0; // Para hacer round robin

uint64_t sys_get_pid() {
    if(current_process == NULL) return -1; // chequeo si el proceso es null

    return current_process->pid;
}

void scheduler() {

    // Limpio los procesos killed, esto capaz se pueda optimizar para no tener que recorrer todo el array 
    for(int i = 0 ; i < MAX_PROCESSES ; i++) {
        if(process_table[i] != NULL && process_table[i]->state == KILLED) {
            //freePage(process_table[i]->rsp);
            process_table[i] = NULL;  
        }
    }

    if(current_process != NULL && current_process->state == RUNNING) {
        // Cambiar el estado del proceso actual a ready o bloqued, hay que estudiarlo un poco
    }

    for(int i = 0 ; i < MAX_PROCESSES ; i++) {
        int next = (last_run_index + i) % MAX_PROCESSES;

        if(process_table[next] != NULL && process_table[next]->state == READY) {
            current_process = process_table[next];
            current_process->state = RUNNING;
            last_run_index = (next + 1) % MAX_PROCESSES;
            return;
        }
    }

    current_process = NULL;
}

void create_process(void * entry_point, const char * process_name) { // Le puse const para que no de warning strlength
    if(entry_point == NULL || process_name == NULL) return;

    void * page = allocate_page();
    if(page == NULL) return; // Si no pudo alocar memoria no se puede crear el proceso
    
    uint64_t * stack = (uint64_t*) ((uint8_t*)page + PAGE_SIZE); // Creacion del stack para el proceso
    pcb_t * new_process = (pcb_t *) page; // Castea la pagina para que se vea como un proceso

    // Esto es inicializacion de registros, hice copy paste de claude
    // Marco de iretq (lo que la CPU restaura) 
    *(--stack) = 0x10;                    // SS  (data segment)
    *(--stack) = (uint64_t)((uint8_t*)page + PAGE_SIZE); // RSP
    *(--stack) = 0x202;                   // RFLAGS (interrupciones habilitadas)
    *(--stack) = 0x08;                    // CS  (code segment)
    *(--stack) = (uint64_t) entry_point;  // RIP

    // Marco de popState (los 15 registros)
    *(--stack) = 0; // rax
    *(--stack) = 0; // rbx
    *(--stack) = 0; // rcx
    *(--stack) = 0; // rdx
    *(--stack) = 0; // rbp
    *(--stack) = 0; // rdi
    *(--stack) = 0; // rsi
    *(--stack) = 0; // r8
    *(--stack) = 0; // r9
    *(--stack) = 0; // r10
    *(--stack) = 0; // r11
    *(--stack) = 0; // r12
    *(--stack) = 0; // r13
    *(--stack) = 0; // r14
    *(--stack) = 0; // r15

    new_process->rsp = (uint64_t) stack;
    new_process->state = READY;
    memcpy(new_process->process_name, process_name, strlenght(process_name+1)); // Si se rompe algo chequear el strlength capaz es por null
        
    for(int i = 0 ; i < MAX_PROCESSES ; i++) {
        if( process_table[i] == NULL) {
            new_process->pid = i+1;
            process_table[i] = new_process;
            return;
        }
    }

    free_page(page); //si no se pudo alocar la memoria libero la page    
}

void exit_process() {
    current_process->state = KILLED;
    //yield() si el scheduler lo llega a elegir que suelte el cpu rapido en alguna iteracion lo va a limpiar, ver despues
}
void block_process(uint64_t pid) {
    if(pid > 0 && process_table[pid-1] != NULL) process_table[pid-1]->state = BLOCKED;
}

void unblock_process(uint64_t pid) {
    if(pid > 0 && process_table[pid-1] != NULL) process_table[pid-1]->state = READY;
}

pcb_t * get_process(uint64_t pid) {
    return process_table[pid-1];
}

uint64_t get_pid_count() {
    uint64_t to_return = 0;
    for(int i = 0 ; i < MAX_PROCESSES ; i++) {
        if(process_table[i] != NULL) to_return++;
    }
}


