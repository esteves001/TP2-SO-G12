#ifndef _SYSCALL_LIB_H
#define _SYSCALL_LIB_H

#include <stdint.h>
#include <timeLib.h>

// Video
extern void sys_clearScreen(void);
extern void sys_putPixel(uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawChar(char c, uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawString(const char *str, uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawRectangle(uint64_t width, uint64_t height, uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawCircle(uint64_t radius, uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawDecimal(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawHexa(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawBin(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y);
extern uint16_t sys_getScreenWidth(void);
extern uint16_t sys_getScreenHeight(void);


// Teclado. 
// TODO: Esto habria que sacarlo creo porque ya tenemos sys_read
extern char sys_kbdGetChar(void);

// Shell
extern uint64_t sys_write(uint8_t fd, const char *str, uint64_t count);
extern uint64_t sys_read(uint8_t fd, char *buffer, uint64_t count);

extern void sys_zoomIn(void);
extern void sys_zoomOut(void);

extern void sys_getDateTime(dateTime *dt);
extern uint64_t sys_getRegisters(uint64_t *registers);
extern void sys_sleepMilli(uint64_t milliseconds);

// Procesos
// Wrappers de las syscalls de procesos. Cada uno corresponde a un case del
// dispatcher en Kernel/syscalls.c (el numero esta en syscallHelper.asm).
extern uint64_t sys_getpid(void);                  // 0x22 - mi pid
extern void     sys_exit(void);                    // 0x23 - termino
extern void     sys_yield(void);                   // 0x24 - cedo CPU
extern int64_t  sys_kill(uint64_t pid);            // 0x25 - mato a otro (-1 si pid invalido)
extern int64_t  sys_block(uint64_t pid);           // 0x26 - bloqueo
extern int64_t  sys_unblock(uint64_t pid);         // 0x27 - desbloqueo

// Struct que el kernel me llena con info de cada proceso para ps.
// IMPORTANTE: definicion duplicada en Kernel/include/process.h.
// Si toco campos aca tambien hay que tocarlos alla (TODO: header compartido).
#define MAX_PROCESS_NAME 32
typedef struct {
    uint64_t pid;
    char name[MAX_PROCESS_NAME];
    int state;              // 0 READY, 1 RUNNING, 2 BLOCKED
    uint64_t rsp;
    uint64_t stack_base;
    uint64_t priority;
    int foreground;
} process_info_t;

// 0x28 - le paso un buffer y el max que entra; me devuelve cuantos llen.
extern int sys_ps(process_info_t * buf, int max);

// 0x30 - cambio la prio de un proceso. 0 si anduvo, -1 si pid o prio invalidos.
// rango valido: 1..5 (MIN_PRIORITY..MAX_PRIORITY en el kernel).
extern int64_t sys_nice(uint64_t pid, uint64_t new_priority);

// 0x31 - userland crea un proceso. devuelve pid (>0) o -1 si fallo.
// entry es un puntero a funcion dentro del modulo userland.
// pipe_in_id/pipe_out_id: id del pipe (1..16) o 0 para ninguno.
extern int64_t sys_create_process(void * entry, const char * name, int argc, char ** argv, int pipe_in_id, int pipe_out_id);
extern void    sys_set_foreground(uint64_t pid, int fg);  // 0x44 - marca fg(1) o bg(0)
extern void    sys_set_fg_pid(uint64_t pid);              // 0x45 - pid que Ctrl+C mata

// Estado del memory manager para el comando mem.
// IMPORTANTE: definicion duplicada en Kernel/include/memoryManager.h.
typedef struct {
    uint64_t total_bytes;   // memoria que el manager puede dar (sin contar kernel/bitmap)
    uint64_t used_bytes;
    uint64_t free_bytes;
} mem_info_t;

// 0x29 - el kernel me rellena el struct con total/used/free.
extern void sys_mem_stats(mem_info_t * out);

extern void opCodeException();

// Semaforos
extern int sys_create_sem(int sem_id, int status, const char * sem_name);
extern int sys_open_sem(int sem_id);
extern void sys_sem_wait(int sem_id);
extern void sys_sem_post(int sem_id);
extern void sys_delete_sem(int sem_id);

// Pipes (ID acordado entre procesos no relacionados, como los semaforos)
extern int  sys_alloc_pipe(void);                     // 0x47 - crea pipe en slot libre, devuelve id (-1 si no hay)
extern int  sys_create_pipe(int id);                  // 0x37 - crea, -1 si ya existe/invalido
extern int  sys_open_pipe(int id);                     // 0x38 - valida que exista
extern int  sys_pipe_read(int id, char * buf, int n);  // 0x39 - lee (bloqueante)
extern int  sys_pipe_write(int id, char * buf, int n); // 0x3A - escribe (bloqueante)
extern void sys_close_pipe(int id);                    // 0x3B - cierra

// Memoria dinamica (userland malloc/free via kernel)
extern void* sys_malloc(uint64_t size);   // 0x41 - reserva bloque
extern void  sys_free(void* ptr);         // 0x42 - libera bloque
extern void  sys_waitpid(uint64_t pid);   // 0x43 - espera que pid termine
extern uint64_t sys_getticks(void);       // 0x46 - ticks desde boot (para busy-wait)

#endif