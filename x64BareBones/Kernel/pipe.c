#include "pipe.h"
#include "process.h"
#include "memoryManager.h"
#include "sem.h"


pipe_t * pipe_create() {
    // El pipe vive en una pagina. Puede que se desperdicie memoria, pero alcanza.
    pipe_t * p = (pipe_t *) allocate_page();
    if(p == NULL) return NULL; // veo la pagina como un pipe
    p->read_pos  = 0;
    p->write_pos = 0;
    p->active    = 1;
    sem_init(&p->mutex, 1);                // mutex arranca libre
    sem_init(&p->empty, PIPE_BUFFER_SIZE); // todo el buffer esta vacio
    sem_init(&p->full, 0);                 // todavia no hay datos

    return p;
}

int pipe_write(pipe_t * p, char * buf, int n) {
    if(p == NULL || p->active == 0) return 0;

    int i;
    for(i = 0; i < n; i++) {
        sem_wait_on(&p->empty);      // espero un espacio libre (bloquea si esta lleno)
        if(p->active == 0) break;    // me cerraron el pipe mientras esperaba
        sem_wait_on(&p->mutex);
        p->buffer[p->write_pos] = buf[i];
        p->write_pos = (p->write_pos + 1) % PIPE_BUFFER_SIZE;
        sem_post_on(&p->mutex);
        sem_post_on(&p->full);       // aviso que hay un dato nuevo
    }

    return i;
}

int pipe_read(pipe_t * p, char * buf, int n) {
    if(p == NULL || p->active == 0) return 0;

    int i;
    for(i = 0; i < n; i++) {
        sem_wait_on(&p->full);       // espero un dato (bloquea si esta vacio)
        if(p->active == 0) break;    // me cerraron el pipe mientras esperaba
        sem_wait_on(&p->mutex);
        buf[i] = p->buffer[p->read_pos];
        p->read_pos = (p->read_pos + 1) % PIPE_BUFFER_SIZE;
        sem_post_on(&p->mutex);
        sem_post_on(&p->empty);      // libere un espacio
    }

    return i;
}

void pipe_close(pipe_t * p) {
    if(p == NULL) return;
    p->active = 0;
    // despierto a cualquiera que estuviera bloqueado para que vea active==0 y salga
    while(p->full.blocked_pids_counter > 0)  sem_post_on(&p->full);
    while(p->empty.blocked_pids_counter > 0) sem_post_on(&p->empty);
    // NO libera: free_page solo lo hace pipe_close_id (unico dueno del pipe por id)
}

// --- Capa por ID: pipes compartidos por un id acordado, como los semaforos ---

static pipe_t * pipe_table[MAX_PIPES] = {0}; // ids 1..MAX_PIPES, indice id-1

int pipe_alloc_id() {
    for (int i = 0; i < MAX_PIPES; i++) {
        if (pipe_table[i] == NULL) {
            pipe_t * p = pipe_create();
            if (p == NULL) return -1;
            pipe_table[i] = p;
            return i + 1; // ids 1..MAX_PIPES
        }
    }
    return -1; // tabla llena
}

int pipe_create_id(int id) {
    if(id < 1 || id > MAX_PIPES) return -1;
    if(pipe_table[id-1] != NULL) return -1;   // ya hay uno con ese id
    pipe_t * p = pipe_create();
    if(p == NULL) return -1;
    pipe_table[id-1] = p;
    return id;
}

int pipe_open_id(int id) {
    if(id < 1 || id > MAX_PIPES || pipe_table[id-1] == NULL) return -1;
    return id;                                 // existe, lo podes usar con ese id
}

int pipe_read_id(int id, char * buf, int n) {
    if(id < 1 || id > MAX_PIPES || pipe_table[id-1] == NULL) return -1;
    return pipe_read(pipe_table[id-1], buf, n);
}

int pipe_write_id(int id, char * buf, int n) {
    if(id < 1 || id > MAX_PIPES || pipe_table[id-1] == NULL) return -1;
    return pipe_write(pipe_table[id-1], buf, n);
}

void pipe_close_id(int id) {
    if(id < 1 || id > MAX_PIPES || pipe_table[id-1] == NULL) return;
    pipe_close(pipe_table[id-1]);
    free_page(pipe_table[id-1]);
    pipe_table[id-1] = NULL;
}

pipe_t * pipe_table_get(int id) {
    if(id < 1 || id > MAX_PIPES) return NULL;
    return pipe_table[id-1];
}
