#include <stdint.h>
#include <usrio.h>
#include <syscallLib.h>
#include "syscall.h"
#include "test_util.h"

enum State { ST_RUNNING,
             ST_BLOCKED,
             ST_KILLED };

typedef struct P_rq {
  int32_t pid;
  enum State state;
} p_rq;

void test_proc(int argc, char *argv[]) {
  uint8_t rq;
  uint8_t alive = 0;
  uint8_t action;
  uint64_t max_processes;
  char *argvAux[] = {0};

  if (argc != 1) { sys_exit(); return; }

  if ((max_processes = satoi(argv[0])) <= 0) { sys_exit(); return; }

  if (max_processes > 64) max_processes = 64;
  p_rq p_rqs[64];

  while (1) {

    // Create max_processes processes
    for (rq = 0; rq < max_processes; rq++) {
      p_rqs[rq].pid = my_create_process("endless_loop", 0, argvAux);

      if (p_rqs[rq].pid == -1) {
        printf("test_processes: ERROR creating process\n");
        sys_exit(); return;
      } else {
        p_rqs[rq].state = ST_RUNNING;
        alive++;
      }
    }

    // Randomly kills, blocks or unblocks processes until every one has been killed
    while (alive > 0) {

      for (rq = 0; rq < max_processes; rq++) {
        action = GetUniform(100) % 2;

        switch (action) {
          case 0:
            if (p_rqs[rq].state == ST_RUNNING || p_rqs[rq].state == ST_BLOCKED) {
              if (my_kill(p_rqs[rq].pid) == -1) {
                printf("test_processes: ERROR killing process\n");
                sys_exit(); return;
              }
              p_rqs[rq].state = ST_KILLED;
              my_wait(p_rqs[rq].pid);
              alive--;
            }
            break;

          case 1:
            if (p_rqs[rq].state == ST_RUNNING) {
              if (my_block(p_rqs[rq].pid) == -1) {
                printf("test_processes: ERROR blocking process\n");
                sys_exit(); return;
              }
              p_rqs[rq].state = ST_BLOCKED;
            }
            break;
        }
      }

      // Randomly unblocks processes
      for (rq = 0; rq < max_processes; rq++)
        if (p_rqs[rq].state == ST_BLOCKED && GetUniform(100) % 2) {
          if (my_unblock(p_rqs[rq].pid) == -1) {
            printf("test_processes: ERROR unblocking process\n");
            sys_exit(); return;
          }
          p_rqs[rq].state = ST_RUNNING;
        }
    }
  }
}