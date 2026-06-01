#include "pipe.h"
#include "process.h"
#include "memoryManager.h"
#include "interrupts.h"


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

    // 1. Si está lleno, me bloqueo esperando que el lector libere espacio
    while(pipe->count == PIPE_BUFFER_SIZE) {
        pipe->waiting_pid = sys_get_pid();
        block_process(pipe->waiting_pid);
        force_schedule();
    }

    // 2. Escribo todo lo que puedo (hasta 'n' o hasta llenar el pipe)
    int written = 0;
    for(int i = 0 ; i < n && pipe->count < PIPE_BUFFER_SIZE ;  i++) {
        pipe->buffer[pipe->write_pos] = buf[i];
        pipe->write_pos = (pipe->write_pos + 1) % PIPE_BUFFER_SIZE; 
        pipe->count++;
        written++;
    }

    // 3. Como acabo de meter datos, el pipe ya no está vacío.
    // Si el lector estaba atrapado esperando leer, lo despierto.
    if(pipe->waiting_pid != 0) {
        unblock_process(pipe->waiting_pid);
        pipe->waiting_pid = 0;
    }

    return written;
}

int pipe_read(pipe_t * pipe, char * buf, int n) {
    if(pipe == NULL || pipe->active == 0) return 0; // No leo nada

    // 1. Si está vacío, me bloqueo esperando que el escritor ponga algo
    while(pipe->count == 0) {
        pipe->waiting_pid = sys_get_pid();
        block_process(pipe->waiting_pid);
        force_schedule(); 
    }

    // 2. Leo todo lo que puedo (hasta 'n' o hasta vaciar el pipe)
    int read = 0;
    for(int i = 0 ; i < n && pipe->count > 0; i++) {
        buf[i] = pipe->buffer[pipe->read_pos];
        pipe->read_pos = (pipe->read_pos + 1) % PIPE_BUFFER_SIZE;
        pipe->count--;
        read++;
    }

    // 3. Como acabo de sacar datos, hay espacio nuevo. 
    // Si el escritor estaba atrapado porque el pipe estaba lleno, lo despierto.
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
