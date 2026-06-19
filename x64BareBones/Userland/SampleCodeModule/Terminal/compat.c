#include "syscall.h"
#include <syscallLib.h>

extern void zero_to_max(int argc, char **argv);
extern void endless_loop(int argc, char **argv);
extern uint64_t my_process_inc(int argc, char **argv);

typedef struct {
    const char *name;
    void       *fn;
} name_entry_t;

static name_entry_t name_table[] = {
    {"zero_to_max",     (void *)0},
    {"endless_loop",    (void *)0},
    {"my_process_inc",  (void *)0},
    {0, 0}
};

static int streq(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;
}

static void *resolve_name(const char *name) {
    name_table[0].fn = (void *)zero_to_max;
    name_table[1].fn = (void *)endless_loop;
    name_table[2].fn = (void *)my_process_inc;

    for (int i = 0; name_table[i].name != 0; i++)
        if (streq(name_table[i].name, name))
            return name_table[i].fn;
    return 0; 
}

int64_t my_create_process(char *name, int argc, char *argv[]) {
    void *fn = resolve_name(name);
    if (fn == 0) return -1;
    return sys_create_process(fn, name, argc, argv, 0, 0);
}

int64_t my_getpid(void)            { return (int64_t)sys_getpid(); }
int64_t my_kill(uint64_t pid)      { return sys_kill(pid); }
int64_t my_block(uint64_t pid)     { return sys_block(pid); }
int64_t my_unblock(uint64_t pid)   { return sys_unblock(pid); }
void    my_yield(void)             { sys_yield(); }

int64_t my_wait(int64_t pid) {
    if (pid < 0) return -1;
    sys_waitpid((uint64_t)pid);
    return pid;
}


int64_t my_nice(uint64_t pid, uint64_t newPrio) {
    uint64_t mapped;
    if (newPrio == 0)      mapped = 1;
    else if (newPrio == 1) mapped = 3;
    else                   mapped = 5;
    return sys_nice(pid, mapped);
}

typedef struct { const char *name; int id; } sem_map_t;

static sem_map_t sem_map[] = {
    {"sem", 1},
    {0, 0}
};

static int sem_name_to_id(const char *name) {
    for (int i = 0; sem_map[i].name != 0; i++)
        if (streq(sem_map[i].name, name))
            return sem_map[i].id;
    return -1;
}

int8_t my_sem_open(char *sem_id, uint64_t initialValue) {
    int id = sem_name_to_id(sem_id);
    if (id < 0) return 0;

    if (sys_create_sem(id, (int)initialValue, sem_id)) return 1;
    return sys_open_sem(id) ? 1 : 0;
}

int8_t my_sem_wait(char *sem_id) {
    int id = sem_name_to_id(sem_id);
    if (id < 0) return 0;
    sys_sem_wait(id);
    return 1;
}

int8_t my_sem_post(char *sem_id) {
    int id = sem_name_to_id(sem_id);
    if (id < 0) return 0;
    sys_sem_post(id);
    return 1;
}

int8_t my_sem_close(char *sem_id) {
    int id = sem_name_to_id(sem_id);
    if (id < 0) return 0;
    return 1;
}

void *malloc(uint64_t size) { return sys_malloc(size); }
void  free(void *ptr)       { sys_free(ptr); }
