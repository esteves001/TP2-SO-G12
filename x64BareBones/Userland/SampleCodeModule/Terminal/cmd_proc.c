#include <cmd_proc.h>
#include <usrio.h>
#include <syscallLib.h>
#include <stringLib.h>

#define TICKS_PER_SEC 18
#define MAX_PS_BUF    16
#define STATE_BLOCKED 2

void cmd_loop(int argc, char **argv) {
    uint64_t pid = sys_getpid();
    while (1) {
        printf("PID: %d looping...\n", (int)pid);
        uint64_t start = sys_getticks();
        while (sys_getticks() - start < TICKS_PER_SEC);
    }
}

void cmd_kill(int argc, char **argv) {
    if (argc < 2) { printf("uso: kill <pid>\n"); sys_exit(); return; }
    int pid = atoi(argv[1]);
    if (sys_kill((uint64_t)pid) < 0)
        printf("error: pid %d no encontrado\n", pid);
    sys_exit();
}

void cmd_nice_cmd(int argc, char **argv) {
    if (argc < 3) { printf("uso: nice <pid> <prioridad 1-5>\n"); sys_exit(); return; }
    int pid  = atoi(argv[1]);
    int prio = atoi(argv[2]);
    if (sys_nice((uint64_t)pid, (uint64_t)prio) < 0)
        printf("error: pid o prioridad invalidos (rango 1-5)\n");
    sys_exit();
}

void cmd_block(int argc, char **argv) {
    if (argc < 2) { printf("uso: block <pid>\n"); sys_exit(); return; }
    int target = atoi(argv[1]);

    process_info_t buf[MAX_PS_BUF];
    int n = sys_ps(buf, MAX_PS_BUF);
    for (int i = 0; i < n; i++) {
        if ((int)buf[i].pid == target) {
            if (buf[i].state == STATE_BLOCKED)
                sys_unblock((uint64_t)target);
            else
                sys_block((uint64_t)target);
            sys_exit();
            return;
        }
    }
    printf("error: pid %d no encontrado\n", target);
    sys_exit();
}
