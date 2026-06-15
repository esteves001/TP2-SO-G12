#ifndef SEM_H_
#define SEM_H_

#include <stdint.h>
#include <stddef.h>

#define MAX_SEM 16
#define MAX_SEM_NAME 32  // largo maximo del nombre de sem(incluyendo el \0)
#define MAX_BLOCKED_PIDS 16  // tope de pids que pueden estar bloqueados en un sem a la vez



typedef struct{
    int id;     // id unico del sem, va de 1 a 16
    int status;    // valor del semaforo -1, 0 ,1 ,2
    int lock;     // 0 si esta released 1 si esta acquired
    char sem_name[MAX_SEM_NAME];
    uint64_t blocked_pids[MAX_BLOCKED_PIDS]; 
    int blocked_pids_counter;   //inicializado en 0
}sem_t;

extern sem_t sem_arr[MAX_SEM]; //array estatico de maximo 16 semaforos creados

void sem_post(int sem_id);
void sem_wait(int sem_id);
int create_sem(int sem_id, int status, const char * sem_name); //devuelve 1 si es valida la creacion 0 sino
void delete_sem(int sem_id);
int open_sem(int sem_id);

// saca un pid de las colas de bloqueados de TODOS los sems. la llamo cuando
// muere un proceso, asi no queda un pid fantasma esperando un post que se
// desperdiciaria. no toma el lock a proposito: corre con IF=0 (la llaman
// exit/kill por int 0x80 y Ctrl+C por IRQ1, ambos interrupt gates) asi que
// en monocore ya es atomico.
void sem_remove_pid(uint64_t pid);

// primitivas que operan sobre un sem_t suelto (no registrado en el array publico).
// las usa el pipe para tener sus propios semaforos sin gastar ids del pool de 16.
void sem_init(sem_t * s, int status);
void sem_wait_on(sem_t * s);
void sem_post_on(sem_t * s);

#endif 