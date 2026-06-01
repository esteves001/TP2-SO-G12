#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>

#define PIPE_BUFFER_SIZE 1024 

typedef struct pipe_struct{
    char buffer[PIPE_BUFFER_SIZE];  
    int read_pos;                   // donde lee p2, avanza modulo PIPE_BUFFER_SIZE
    int write_pos;                  // donde escribe p1, avanza modulo PIPE_BUFFER_SIZE
    int count;                      // bytes disponibles para leer, 0 = vacio, PIPE_BUFFER_SIZE = lleno
    int active;                     // 1 si el pipe existe, 0 si fue cerrado
    uint64_t waiting_pid;           // pid del proceso bloqueado esperando datos, 0 si nadie espera
} pipe_t;

pipe_t * pipe_create();                           // aloca e inicializa el pipe
int pipe_write(pipe_t * pipe, char * buf, int n); // escribe n bytes al pipe
int pipe_read(pipe_t * pipe, char * buf, int n);  // lee n bytes del pipe
void pipe_close(pipe_t * pipe);                   // cierra y libera el pipe

#endif