#include <stdint.h>
#include <usrio.h>
#include <syscallLib.h>
#include "syscall.h"
#include "test_util.h"

#define TOTAL_PROCESSES 3

#define LOWEST 0
#define MEDIUM 1
#define HIGHEST 2

int64_t prio[TOTAL_PROCESSES] = {LOWEST, MEDIUM, HIGHEST};

uint64_t max_value = 0;

void zero_to_max(int argc, char **argv) {
  uint64_t value = 0;

  while (value++ != max_value);

  printf("PROCESS %d DONE!\n", (int)my_getpid());
  sys_exit();
}

void test_prio(int argc, char *argv[]) {
  int64_t pids[TOTAL_PROCESSES];
  char *ztm_argv[] = {0};
  uint64_t i;

  if (argc != 1) { sys_exit(); return; }

  if ((max_value = satoi(argv[0])) <= 0) { sys_exit(); return; }

  printf("SAME PRIORITY...\n");

  for (i = 0; i < TOTAL_PROCESSES; i++)
    pids[i] = my_create_process("zero_to_max", 0, ztm_argv);

  for (i = 0; i < TOTAL_PROCESSES; i++)
    my_wait(pids[i]);

  printf("SAME PRIORITY, THEN CHANGE IT...\n");

  for (i = 0; i < TOTAL_PROCESSES; i++) {
    pids[i] = my_create_process("zero_to_max", 0, ztm_argv);
    my_nice(pids[i], prio[i]);
    printf("  PROCESS %d NEW PRIORITY: %d\n", (int)pids[i], (int)prio[i]);
  }

  for (i = 0; i < TOTAL_PROCESSES; i++)
    my_wait(pids[i]);

  printf("SAME PRIORITY, THEN CHANGE IT WHILE BLOCKED...\n");

  for (i = 0; i < TOTAL_PROCESSES; i++) {
    pids[i] = my_create_process("zero_to_max", 0, ztm_argv);
    my_block(pids[i]);
    my_nice(pids[i], prio[i]);
    printf("  PROCESS %d NEW PRIORITY: %d\n", (int)pids[i], (int)prio[i]);
  }

  for (i = 0; i < TOTAL_PROCESSES; i++)
    my_unblock(pids[i]);

  for (i = 0; i < TOTAL_PROCESSES; i++)
    my_wait(pids[i]);

  sys_exit();
}