#include "process.h"
#include "memoryManager.h"
#include "lib.h"
#include "interrupts.h" // para _hlt() en idle_process
#include "timeLib.h"    // para timer_handler() en schedule_tick
#include "pipe.h"
#include "sem.h"      // para sem_remove_pid al matar un proceso
#include "syscalls.h" // para Registers_t (mapeo de los regs que guarda pushState)

// definiciones de los globales declarados extern en process.h
// arrancan en NULL/0 porque al boot todavia no hay ningun proceso
pcb_t * current_process = NULL;
pcb_t * process_table[MAX_PROCESSES] = {0}; // todos los slots libres al principio

static int last_run_index = 0; // Para hacer round robin

// puntero al PCB de idle. lo guardo aca para reconocerlo en el scheduler
// (idle es caso especial: solo corre si no hay nadie mas READY).
static pcb_t * idle_pcb = NULL;

// proceso idle: corre cuando no hay nadie mas READY
// hlt duerme al CPU hasta la proxima interrupcion (no quema CPU al pedo)
static void idle_process() {
    while(1) {
        _hlt();
    }
}

// helper publico para crear el idle, idle_process es static a este archivo
void create_idle_process() {
    create_process(&idle_process, "idle", 0, NULL, 0, 0);
    idle_pcb = process_table[0];
    idle_pcb->foreground = 0;
}

// copio argc/argv adentro del page del proceso. Layout:
//   [pcb_t][argv_ptrs (MAX_ARGS+1 punteros)][strings concatenados][...][stack]
// devuelvo el puntero al array de argv_ptrs (lo que ira a RSI cuando arranque).
// el +1 en el array es para el NULL terminator (POSIX: argv[argc] == NULL,
// para poder iterar sin saber argc de antemano).
static char ** copy_argv_to_page(void * page, int argc, char ** argv) {
    if(argc <= 0 || argv == NULL) return NULL;
    if(argc > MAX_ARGS) argc = MAX_ARGS; // si me mandan mas, los recorto

    // alineo a 8 para que el array de punteros quede aligned
    uint64_t off = (sizeof(pcb_t) + 7) & ~7ULL;
    char ** ptrs = (char **)((uint8_t *)page + off);
    char * strings = (char *)(ptrs + MAX_ARGS + 1); // strings van despues del array

    for(int i = 0; i < argc; i++) {
        ptrs[i] = strings;
        int j = 0;
        while(j < MAX_ARG_LEN - 1 && argv[i][j] != 0) {
            *strings++ = argv[i][j];
            j++;
        }
        *strings++ = 0; // terminador del string
    }
    ptrs[argc] = NULL; // POSIX: argv[argc] = NULL
    return ptrs;
}

// llamado desde _irq00Handler con el rsp actual del proceso que se interrumpio.
// guarda ese rsp en el PCB, corre el scheduler, y devuelve el rsp del proximo.
uint64_t schedule_tick(uint64_t current_rsp) {
    if(current_process != NULL) {
        current_process->rsp = current_rsp;
    }
    timer_handler(); // ticks++, para que sleep() siga andando

    // si el actual sigue RUNNING y no es idle, le saco un tick. si todavia le
    // quedan, sigo con el mismo (no hace falta llamar al scheduler). asi un
    // proceso con prio 5 corre 5 ticks seguidos antes de soltar.
    if(current_process != NULL && current_process->state == RUNNING
       && current_process != idle_pcb && current_process->ticks_remaining > 1) {
        current_process->ticks_remaining--;
        return current_process->rsp;
    }

    scheduler();     // elige el nuevo current_process
    return current_process->rsp;
}

uint64_t sys_get_pid() {
    if(current_process == NULL) return -1; // chequeo si el proceso es null

    return current_process->pid;
}

void scheduler() {

    // Limpio los procesos killed, esto capaz se pueda optimizar para no tener que recorrer todo el array
    for(int i = 0 ; i < MAX_PROCESSES ; i++) {
        if(process_table[i] != NULL && process_table[i]->state == KILLED) {
            // si alguien mato a idle (no deberia pasar nunca), limpio el puntero
            if(process_table[i] == idle_pcb) idle_pcb = NULL;
            free_page(process_table[i]);
            process_table[i] = NULL;
        }
    }

    if(current_process != NULL && current_process->state == RUNNING) {
        current_process->state = READY; // Si esta corriendo directamente lo pongo en ready sino el proceso se bloquea solo
    }
    // Primero busco un READY que NO sea idle (Round Robin).
    // A idle lo trato como fallback: solo corre si no encontre a nadie mas.
    for(int i = 0 ; i < MAX_PROCESSES ; i++) {
        int next = (last_run_index + i) % MAX_PROCESSES;
        pcb_t * p = process_table[next];

        if(p != NULL && p != idle_pcb && p->state == READY) {
            current_process = p;
            current_process->state = RUNNING;
            // arranca su turno con tantos ticks como su prio
            current_process->ticks_remaining = current_process->priority;
            last_run_index = (next + 1) % MAX_PROCESSES;
            return;
        }
    }

    // no hay nadie READY -> corro idle. nunca se bloquea ni se mata,
    // asi que siempre esta disponible como salvavidas del scheduler.
    if(idle_pcb != NULL) {
        current_process = idle_pcb;
        current_process->state = RUNNING;
        current_process->ticks_remaining = 1; // a idle no le doy quantum largo
        return;
    }

    current_process = NULL; // no deberia pasar nunca
}

int64_t create_process(void * entry_point, const char * process_name, int argc, char ** argv, int pipe_in_id, int pipe_out_id) {
    if(entry_point == NULL || process_name == NULL) return -1;

    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i] != NULL && process_table[i]->state == KILLED) {
            if(process_table[i] == idle_pcb) idle_pcb = NULL;
            free_page(process_table[i]);
            process_table[i] = NULL;
        }
    }

    void * page = allocate_page();
    if(page == NULL) return -1; // Si no pudo alocar memoria no se puede crear el proceso

    if(argc < 0) argc = 0;
    if(argc > MAX_ARGS) argc = MAX_ARGS;
    char ** argv_in_page = copy_argv_to_page(page, argc, argv);

    uint64_t * stack = (uint64_t*) ((uint8_t*)page + PAGE_SIZE); // Creacion del stack para el proceso
    pcb_t * new_process = (pcb_t *) page; // Castea la pagina para que se vea como un proceso

    // Esto es inicializacion de registros
    // Marco de iretq (lo que la CPU restaura)
    *(--stack) = 0x0;                    // SS  (data segment)
    *(--stack) = (uint64_t)((uint8_t*)page + PAGE_SIZE); // RSP
    *(--stack) = 0x202;                   // RFLAGS (interrupciones habilitadas)
    *(--stack) = 0x08;                    // CS  (code segment)
    *(--stack) = (uint64_t) entry_point;  // RIP


    *(--stack) = 0; // rax
    *(--stack) = 0; // rbx
    *(--stack) = 0; // rcx
    *(--stack) = 0; // rdx
    *(--stack) = 0; // rbp
    *(--stack) = (uint64_t) argc;         // rdi  (1er arg)
    *(--stack) = (uint64_t) argv_in_page; // rsi  (2do arg) -- NULL si argc==0
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
    // copio el nombre acotado al tamaño del buffer, dejando lugar para el \0
    int i = 0;
    while(i < MAX_PROCESS_NAME - 1 && process_name[i] != 0) {
        new_process->process_name[i] = process_name[i];
        i++;
    }
    new_process->process_name[i] = 0; // terminador
    new_process->pipe_in  = (pipe_in_id  > 0) ? pipe_table_get(pipe_in_id)  : NULL;
    new_process->pipe_out = (pipe_out_id > 0) ? pipe_table_get(pipe_out_id) : NULL;
    new_process->priority = MIN_PRIORITY;
    new_process->ticks_remaining = MIN_PRIORITY;
    new_process->foreground = 1;
    new_process->waiting_for_pid = 0;
    new_process->text_color = 0xFFFFFF; // blanco por defecto, igual que hasta ahora

    for(int i = 0 ; i < MAX_PROCESSES ; i++) {
        if( process_table[i] == NULL) {
            new_process->pid = i+1;
            process_table[i] = new_process;
            return (int64_t)new_process->pid;
        }
    }

    free_page(page); // tabla llena, libero la page que ya habia reservado
    return -1;
}

// Mata al proceso que le pase. Lo cambie para que reciba un PCB en vez de
// trabajar siempre sobre current_process: asi me sirve para exit (paso current)
// y para kill(pid) (paso process_table[pid-1]).
// Notar que NO libero la pagina aca: solo marco KILLED. El scheduler la libera
// la proxima vez que corra (ver el for de arriba en scheduler()). Esto es
// importante porque si me mato a mi mismo, todavia estoy corriendo en mi
// propio stack -> no lo puedo liberar todavia.
void exit_process(pcb_t * proc) {
    if(proc == NULL) return;
    pipe_close(proc->pipe_in);
    pipe_close(proc->pipe_out);
    proc->state = KILLED;

    // lo saco de las colas de los sems asi no queda un pid fantasma que se
    // coma un sem_post (sino un proceso vivo que esperaba nunca despierta)
    sem_remove_pid(proc->pid);

    // desbloqueo a quien estaba esperando este proceso
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i] != NULL && process_table[i]->waiting_for_pid == proc->pid) {
            process_table[i]->waiting_for_pid = 0;
            unblock_process(process_table[i]->pid);
        }
    }
}

void waitpid(uint64_t pid) {
    if(pid == 0 || pid > MAX_PROCESSES || process_table[pid-1] == NULL) return;
    if(process_table[pid-1]->state == KILLED) return;

    current_process->waiting_for_pid = pid;
    block_process(current_process->pid);
    force_schedule();
}
void block_process(uint64_t pid) {
    if(pid > 0 && process_table[pid-1] != NULL) process_table[pid-1]->state = BLOCKED;
}

void unblock_process(uint64_t pid) {
    // Si esta dormido lo despierto 
    if(pid > 0 && process_table[pid-1] != NULL && process_table[pid-1]->state == BLOCKED) {
        process_table[pid-1]->state = READY;
    }
}

// cambio la prio. devuelve -1 si pid invalido o prio fuera de [MIN, MAX].
// NO toco ticks_remaining: el cambio se ve recien la proxima vez que el
// scheduler lo elija, sino seria injusto cortarle el turno actual.
int set_priority(uint64_t pid, uint64_t new_priority) {
    if(pid == 0 || pid > MAX_PROCESSES || process_table[pid-1] == NULL) return -1;
    if(new_priority < MIN_PRIORITY || new_priority > MAX_PRIORITY) return -1;
    process_table[pid-1]->priority = new_priority;
    return 0;
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

// Recorro la process_table y voy llenando el buffer del user con la info de
// cada proceso vivo. Salto los NULL (slots libres) y los KILLED (zombis que
// el scheduler todavia no limpio). Devuelvo cuantos escribi para que el user
// sepa hasta donde leer. El "max" es el tope del buffer: si tiene menos
// capacidad que MAX_PROCESSES, corto. Asi nunca me paso del buffer.
int ps_snapshot(process_info_t * buf, int max) {
    if(buf == NULL || max <= 0) return 0;
    int written = 0;
    for(int i = 0; i < MAX_PROCESSES && written < max; i++) {
        pcb_t * p = process_table[i];
        if(p == NULL || p->state == KILLED) continue;
        process_info_t * out = &buf[written];
        out->pid = p->pid;
        // copio el nombre a mano (no tengo strncpy en kernel) acotando al buffer
        int j = 0;
        while(j < MAX_PROCESS_NAME - 1 && p->process_name[j] != 0) {
            out->name[j] = p->process_name[j];
            j++;
        }
        out->name[j] = 0;  // terminador
        out->state = p->state;
        out->rsp = p->rsp;
        // el RBP del proceso esta guardado en su propio stack: lo salva el
        // context switch. p->rsp apunta al bloque de registros, asi que lo
        // leo con el mismo mapeo que usa pushState.
        out->rbp = ((Registers_t *)p->rsp)->rbp;
        out->priority = p->priority;
        out->foreground = p->foreground;
        written++;
    }
    return written;
}




