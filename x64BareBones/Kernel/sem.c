#include "sem.h"
#include "lib.h"
#include "process.h"
#include "interrupts.h"

sem_t sem_arr[MAX_SEM] = {0};
//en todo el codigo uso sem_id-1 porque los ids van de 1 a 16 por tanto en el arreglo van de 0 a 15

int valid_sem_id(int sem_id) { // algo hay que validar seguramente no se que es aun, por lo pronto puse que sea un sem_id valido
    return sem_id >= 1 && sem_id <= 16;
}

// inicializa un sem suelto (el del pipe). la pagina viene con basura, asi que
// pongo a mano lo que importa para wait/post: status, lock y la cola.
void sem_init(sem_t * s, int status) {
    s->id = 0;            // 0 = no esta registrado en el array publico
    s->status = status;
    s->lock = 0;
    s->blocked_pids_counter = 0;
}

void sem_post_on(sem_t * s) {
    acquire(&s->lock);
    s->status++;
    if(s->status <= 0) {
        uint64_t to_unblock_pid = s->blocked_pids[0];
        for(int i = 0; i < s->blocked_pids_counter - 1; i++) {
            s->blocked_pids[i] = s->blocked_pids[i+1];
        }
        s->blocked_pids[--s->blocked_pids_counter] = 0;
        unblock_process(to_unblock_pid);
    }
    release(&s->lock);
}

void sem_wait_on(sem_t * s) {
    acquire(&s->lock);
    s->status--;
    if(s->status < 0) {
        if(s->blocked_pids_counter >= MAX_BLOCKED_PIDS) {
            s->status++;
            release(&s->lock);
            return;
        }
        uint64_t mi_pid = sys_get_pid();
        s->blocked_pids[s->blocked_pids_counter++] = mi_pid;
        block_process(mi_pid); //primero bloqueo
        release(&s->lock); // luego suelto
        force_schedule(); // luego cambio de proceso
        return;
    }
    release(&s->lock);
}

// las versiones por id ahora resuelven id->puntero y delegan en las primitivas
void sem_post(int sem_id) {
    if(!valid_sem_id(sem_id)) return;
    sem_post_on(&sem_arr[sem_id-1]);
}

void sem_wait(int sem_id) {
    if(!valid_sem_id(sem_id)) return;
    sem_wait_on(&sem_arr[sem_id-1]);
}

int create_sem(int sem_id, int status, const char * sem_name) { // tambien chequear ya no tener un sem creado con el mismo id, por lo pronto asumo que no luego valido
    if(valid_sem_id(sem_id)) {
        if(sem_arr[sem_id-1].id != 0) {
            return 0; 
        }
        
        sem_t new_sem = {0}; 
        new_sem.id = sem_id;
        new_sem.status = status;
        memcpy(new_sem.sem_name, sem_name, strlenght(sem_name)+1);
        sem_arr[sem_id-1] = new_sem;
        return 1;
    }
    return 0; // Error por id invalido
}

void delete_sem(int sem_id) {
    if(valid_sem_id(sem_id)) {

        // despierto a todos los procesos atrapados 
        for(int i = 0; i < sem_arr[sem_id-1].blocked_pids_counter; i++) {
            uint64_t pid_durmiendo = sem_arr[sem_id-1].blocked_pids[i];
            unblock_process(pid_durmiendo);
        }

        sem_arr[sem_id-1].blocked_pids_counter = 0;
        sem_arr[sem_id-1].lock = 0;
        sem_arr[sem_id-1].status = 0;
        sem_arr[sem_id-1].id = 0; 
        memset(sem_arr[sem_id-1].sem_name, 0, MAX_SEM_NAME);
        memset(sem_arr[sem_id-1].blocked_pids, 0, sizeof(sem_arr[sem_id-1].blocked_pids));
    }
}

int open_sem(int sem_id) {
    if(valid_sem_id(sem_id)) {
        if(sem_arr[sem_id-1].id == sem_id) {
            return sem_id;
        }
    }
    return 0;
}