#include <stdint.h>
#include <usrio.h>    /* printf */
#include <syscallLib.h>
#include "syscall.h"
#include "test_util.h"

#define SEM_ID "sem"
#define TOTAL_PAIR_PROCESSES 2

int64_t global; // shared memory

void slowInc(int64_t *p, int64_t inc) {
  uint64_t aux = *p;
  if (GetUniform(100) < 30)
    my_yield(); // This makes the race condition highly probable
  aux += inc;
  *p = aux;
}

// entry-point
uint64_t my_process_inc(int argc, char *argv[]) {
  uint64_t n;
  int8_t inc;
  int8_t use_sem;

  if (argc != 3) { sys_exit(); return -1; }

  if ((n = satoi(argv[0])) <= 0) { sys_exit(); return -1; }
  if ((inc = satoi(argv[1])) == 0) { sys_exit(); return -1; }
  if ((use_sem = satoi(argv[2])) < 0) { sys_exit(); return -1; }

  if (use_sem)
    if (!my_sem_open(SEM_ID, 1)) {
      printf("test_sync: ERROR opening semaphore\n");
      sys_exit();
      return -1;
    }

  uint64_t i;
  for (i = 0; i < n; i++) {
    if (use_sem)
      my_sem_wait(SEM_ID);
    slowInc(&global, inc);
    if (use_sem)
      my_sem_post(SEM_ID);
  }

  if (use_sem)
    my_sem_close(SEM_ID);

  sys_exit();
  return 0;
}

void test_sync(int argc, char *argv[]) { //{n, use_sem}
  uint64_t pids[2 * TOTAL_PAIR_PROCESSES];

  if (argc != 2) { sys_exit(); return; }

  char *argvDec[] = {argv[0], "-1", argv[1], 0};
  char *argvInc[] = {argv[0], "1", argv[1], 0};

  global = 0;

  uint64_t i;
  for (i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
    pids[i] = my_create_process("my_process_inc", 3, argvDec);
    pids[i + TOTAL_PAIR_PROCESSES] = my_create_process("my_process_inc", 3, argvInc);
  }

  for (i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
    my_wait(pids[i]);
    my_wait(pids[i + TOTAL_PAIR_PROCESSES]);
  }

  printf("Final value: %d\n", (int)global);

  sys_exit();
}
