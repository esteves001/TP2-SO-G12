#include "process.h"

uint64_t sys_get_pid() {
    if(current_process == NULL) return -1; // chequeo si el proceso es null

    return current_process->pid;
}
