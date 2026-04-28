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
            free_page(process_table[i]);
            process_table[i] = NULL;  
        }
    }

    if(current_process != NULL && current_process->state == RUNNING) {
        current_process->state = READY; // Si esta corriendo directamente lo pongo en ready sino el proceso se bloquea solo
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
    *(--stack) = 0x0;                    // SS  (data segment)
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
    memcpy(new_process->process_name, process_name, strlenght(process_name)+1); // Si se rompe algo chequear el strlength capaz es por null
    new_process->pipe_in = NULL;
    new_process->pipe_out = NULL;
        
    for(int i = 0 ; i < MAX_PROCESSES ; i++) {
        if( process_table[i] == NULL) {
            new_process->pid = i+1;
            process_table[i] = new_process;
            return;
        }
    }

    free_page(page); //si no se pudo alocar la memoria libero la page  
    // agregar int 0x20 forzar a cortar al timertick, esto es para que si agrego un proceso con prioridad se ejecute ese.  
}

void exit_process() {
    pipe_close(current_process->pipe_in);
    pipe_close(current_process->pipe_out);
    current_process->state = KILLED;
    //yield() si el scheduler lo llega a elegir que suelte el cpu rapido en alguna iteracion lo va a limpiar, ver despues que onda
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

    return to_return;
}

pipe_t * pipe_create() { 
    // En este caso al pipe se le da el espacio de una pagina, puede ser que se desperdicie bastante memoria
    void * page = allocate_page();
    if(page == NULL) return NULL;
    
    pipe_t * new_pipe = (pipe_t *)page; // Es importante el casteo para que la pagina sea vista como un pipe

    new_pipe->read_pos = 0;
    new_pipe->write_pos = 0;
    new_pipe->count = 0;
    new_pipe->active = 1;
    new_pipe->waiting_pid = 0;
    
    return new_pipe;
}

int pipe_write(pipe_t * pipe, char * buf, int n) {
    if(pipe == NULL || pipe->active == 0) return 0; // No escribio nada

    int written = 0;

    for(int i = 0 ; i < n && pipe->count < PIPE_BUFFER_SIZE ;  i++) {
        pipe->buffer[pipe->write_pos] = buf[i];
        pipe->write_pos = (pipe->write_pos + 1) % PIPE_BUFFER_SIZE; //esto es para que escriba de manera circular, no se si esta bien
        pipe->count++;
        written++;
    }

    if(pipe->waiting_pid != 0) {
        unblock_process(pipe->waiting_pid);
        pipe->waiting_pid = 0;
    }

    return written;
}

int pipe_read(pipe_t * pipe, char * buf, int n) {
    if(pipe == NULL || pipe->active == 0) return 0; // No leo nada

    int read = 0;

    for(int i = 0 ; i < n && pipe->count > 0; i++) {
        buf[i] = pipe->buffer[pipe->read_pos];
        pipe->read_pos = (pipe->read_pos + 1) % PIPE_BUFFER_SIZE;
        pipe->count--;
        read++;
    }

    if(pipe->waiting_pid != 0) {
        unblock_process(pipe->waiting_pid);
        pipe->waiting_pid = 0;
    }

    return read;
}

void pipe_close(pipe_t* pipe) {
    if(pipe == NULL) return;
    pipe->active = 0;
    if(pipe->waiting_pid != 0) {
        unblock_process(pipe->waiting_pid);
        pipe->waiting_pid = 0;
    }
    free_page(pipe);
}

