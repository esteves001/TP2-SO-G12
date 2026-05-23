#include "sem.h"
#include "lib.h"

sem_t sem_arr[MAX_SEM] = {0};

void sem_post(int sem_id) { 
    if(valid_sem_id(sem_id) && sem_arr[sem_id-1].lock == 0) {
        sem_arr[sem_id-1].status++;
        return;
    }
}

void sem_wait(int sem_id) { // idem post
    if(valid_sem_id(sem_id) && sem_arr[sem_id-1].lock == 0)
        sem_arr[sem_id-1].status--;
}

int create_sem(int sem_id, int status, const char * sem_name) { // tambien chequear ya no tener un sem creado con el mismo id, por lo pronto asumo que no luego valido
    sem_t new_sem;
    new_sem.id = sem_id;
    new_sem.lock = 0;
    new_sem.blocked_pids_counter = 0;
    new_sem.status = status;
    memcpy(new_sem.sem_name, sem_name, strlenght(sem_name)+1);
    sem_arr[sem_id-1] = new_sem;
    return 1;
}

void delete_sem(int sem_id) {
    for(int i = 0 ; i < MAX_SEM ; i++) {
        if(sem_arr[i].id == sem_id-1) {
            sem_arr[i].id = 0; 
            return;
        }
    }    
}

static int valid_sem_id(int sem_id) {
    return sem_id >= 1 && sem_id <= 16;
}