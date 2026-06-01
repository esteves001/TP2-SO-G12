#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>
#include "sem.h"

#define PIPE_BUFFER_SIZE 1024

typedef struct pipe_struct{
    char buffer[PIPE_BUFFER_SIZE];
    int read_pos;                   // donde lee el lector, avanza modulo PIPE_BUFFER_SIZE
    int write_pos;                  // donde escribe el escritor, avanza modulo PIPE_BUFFER_SIZE
    int active;                     // 1 si el pipe existe, 0 si fue cerrado
    sem_t mutex;                    // protege buffer y posiciones (arranca en 1)
    sem_t empty;                    // espacios libres (arranca en PIPE_BUFFER_SIZE)
    sem_t full;                     // bytes listos para leer (arranca en 0)
} pipe_t;

pipe_t * pipe_create();                           // aloca e inicializa el pipe
int pipe_write(pipe_t * pipe, char * buf, int n); // escribe n bytes al pipe
int pipe_read(pipe_t * pipe, char * buf, int n);  // lee n bytes del pipe
void pipe_close(pipe_t * pipe);                   // cierra y libera el pipe

// Capa por ID: procesos no relacionados comparten un pipe con un id acordado,
// igual que los semaforos. ids 1..MAX_PIPES.
#define MAX_PIPES 16

int  pipe_create_id(int id);                    // crea el pipe con ese id (-1 si ya existe/invalido)
int  pipe_open_id(int id);                       // valida que exista para usarlo
int  pipe_read_id(int id, char * buf, int n);    // lee del pipe id (bloqueante)
int  pipe_write_id(int id, char * buf, int n);   // escribe al pipe id (bloqueante)
void pipe_close_id(int id);                      // cierra y libera el pipe id

#endif