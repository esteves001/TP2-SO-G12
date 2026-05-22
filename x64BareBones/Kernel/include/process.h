#ifndef PROCESS_H_
#define PROCESS_H_

#include <stdint.h>
#include <stddef.h>

#define MAX_PROCESSES 16
#define PIPE_BUFFER_SIZE 256
#define MAX_PROCESS_NAME 32  // largo maximo del nombre de un proceso (incluyendo el \0)
 
// rango de prioridades. el numero es cuantos ticks corre el proceso por turno.
// prio alta -> corre mas seguido, prio baja -> menos. default es la minima.
#define MIN_PRIORITY 1  
#define MAX_PRIORITY 5

// limites del area de argv adentro del page del proceso
#define MAX_ARGS 8       // hasta 8 args (argv[0]..argv[7]), alcanza para shell
#define MAX_ARG_LEN 32   // cada arg hasta 31 chars + \0
 
typedef enum { READY = 0, RUNNING, BLOCKED, KILLED } process_state;

typedef struct {
    char buffer[PIPE_BUFFER_SIZE];  
    int read_pos;                   // donde lee p2, avanza modulo PIPE_BUFFER_SIZE
    int write_pos;                  // donde escribe p1, avanza modulo PIPE_BUFFER_SIZE
    int count;                      // bytes disponibles para leer, 0 = vacio, PIPE_BUFFER_SIZE = lleno
    int active;                     // 1 si el pipe existe, 0 si fue cerrado
    uint64_t waiting_pid;           // pid del proceso bloqueado esperando datos, 0 si nadie espera
} pipe_t;

typedef struct {
    uint64_t pid;       // Pid del proceso
    uint64_t rsp;       // Puntero al stack para el scheduler
    char process_name[MAX_PROCESS_NAME]; // array fijo, copiamos el nombre adentro del PCB
    int state;      // Estado del proceso(blocked, running, ready, killed es un estado inventado para saber cuando se debe retirar un proceso)

    pipe_t * pipe_in;   // pipe del que lee
    pipe_t * pipe_out;  // pipe al que escribe

    uint64_t priority;        // 1..5, default 1
    uint64_t ticks_remaining; // cuantos ticks me quedan en este turno
} pcb_t;

// Struct que devuelvo al userland en la syscall ps. NO es el PCB: es un
// snapshot acotado con solo los campos que el user puede ver. Asi no expongo
// punteros internos del kernel ni dependencias raras.
// IMPORTANTE: esta misma definicion esta duplicada en syscallLib.h (userland).
// Si toco campos aca tengo que tocarlos alla tambien (TODO: header compartido).
typedef struct {
    uint64_t pid;
    char name[MAX_PROCESS_NAME];
    int state;              // READY/RUNNING/BLOCKED (KILLED no aparece, lo filtro)
    uint64_t rsp;
    uint64_t stack_base;    // tope del stack = pcb + PAGE_SIZE
    uint64_t priority;      // placeholder hasta que existan prioridades
    int foreground;         // placeholder hasta que exista fg/bg
} process_info_t;

extern pcb_t* current_process; // Puntero al proceso actual corriendo
extern pcb_t* process_table[MAX_PROCESSES]; // Por lo pronto tenemos el array estatico con max 16 procesos y ordenados por pid (pid = 1 -> process_table[0], ..)

uint64_t sys_get_pid();
void scheduler();

// Funcionalidades de procesos
// argc/argv pueden ser 0/NULL si el proceso no usa args (ej: idle).
// Las strings se COPIAN dentro de la pagina del proceso, no me quedo con
// punteros al stack del caller.
// devuelve el pid creado (>0) o -1 si fallo.
int64_t create_process(void * entry_point, const char * process_name, int argc, char ** argv);
void create_idle_process();   // crea el proceso idle (necesario al boot)
// Mata el proceso dado. Recibe el PCB para servir tanto a exit (paso current)
// como a kill(pid) (paso process_table[pid-1]).
void exit_process(pcb_t * proc);
void block_process(uint64_t pid);
void unblock_process(uint64_t pid);

// cambio la prio de un proceso. devuelve 0 si anduvo, -1 si pid o prio invalidos.
int set_priority(uint64_t pid, uint64_t new_priority);

// Primer salto al primer proceso (implementado en asm). No vuelve nunca.
// rsp = rsp del PCB del primer proceso
void start_first_process(uint64_t rsp);

// llamada desde el handler del timer tick (asm) para hacer context switch.
// recibe el rsp del proceso saliente, devuelve el rsp del proceso entrante.
uint64_t schedule_tick(uint64_t current_rsp);

// Funciones utiles segun claude
pcb_t* get_process(uint64_t pid); // Buscar proceso por pid, util internamente
uint64_t get_pid_count();         // Cuantos procesos hay corriendo, util para debug

// Llena buf con snapshot de los procesos vivos (no KILLED).
// Devuelve cuantos escribio (acotado por max). Lo usa la syscall 0x28.
int ps_snapshot(process_info_t * buf, int max);


// Funcionalidades de pipes
pipe_t * pipe_create();                          // aloca e inicializa el pipe
int pipe_write(pipe_t * pipe, char * buf, int n); // escribe n bytes al pipe
int pipe_read(pipe_t * pipe, char * buf, int n);  // lee n bytes del pipe
void pipe_close(pipe_t * pipe);                  // cierra y libera el pipe

#endif  