#include "process.h"

static int last_run_index = 0; // Para hacer round robin

uint64_t sys_get_pid() {
    if(current_process == NULL) return -1; // chequeo si el proceso es null

    return current_process->pid;
}

void scheduler() {

    // Limpio los procesos killed
    for(int i = 0 ; i < MAX_PROCESSES ; i++) {
        if(process_table[i] != NULL && process_table[i]->state == KILLED) {
            process_table[i] = NULL; 
        }
    }

    for(int i = 0 ; i < MAX_PROCESSES ; i++) {
        int next = (last_run_index + i) % MAX_PROCESSES;

        if(process_table[next] != NULL && process_table[next]->state == READY) {

            if(current_process != NULL && current_process->state == RUNNING) {
                // Cambiar el estado del proceso actual a ready o bloqued, hay que estudiarlo un poco
            }

            last_run_index = next;
            current_process = process_table[next];
            current_process->state = RUNNING;

            return;
        }
    }
    
}

void create_process(void * entry_point, char * process_name) { //terminar esta medio exotica
    if(entry_point == NULL || process_name == NULL) return;

    pcb_t * new_process;

    for(int i = 0 ; i < MAX_PROCESSES ; i++) {
        if( process_table[i] == NULL) {

            new_process->pid = i+1;
            //new_process->process_name = process_name; 
            //new_process->rsp = 
            new_process->state = READY;

            process_table[i] = new_process;
            return;
        }
    }
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


