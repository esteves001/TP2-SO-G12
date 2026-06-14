#include <stdint.h>
#include <test_util.h>
#include <usrio.h>
#include <syscallLib.h>

#define MUTEX_SEM_ID 1
#define MAX_PAIRS 6      // idle+shell+test_sync = 3 slots; quedan 13 -> 6 pares (12 hijos)

// variable compartida entre todos los procesos. en este kernel todos los
// procesos de userland comparten el mismo address space, asi que una global
// es compartida de verdad (ese es justo el punto del test de races).
static volatile int64_t global_balance = 0;

// seccion critica: leo, cedo la CPU a proposito (fuerzo el race) y escribo.
// con sem la protejo; sin sem se ve la corrupcion.
static void inc_process(int argc, char ** argv) {
    uint64_t loops = satoi(argv[0]);
    int use_sem = satoi(argv[1]);
    if (use_sem) sys_open_sem(MUTEX_SEM_ID);

    for (uint64_t i = 0; i < loops; i++) {
        if (use_sem) sys_sem_wait(MUTEX_SEM_ID);
        int64_t tmp = global_balance;
        sys_yield();
        global_balance = tmp + 1;
        if (use_sem) sys_sem_post(MUTEX_SEM_ID);
    }
    sys_exit();
}

static void dec_process(int argc, char ** argv) {
    uint64_t loops = satoi(argv[0]);
    int use_sem = satoi(argv[1]);
    if (use_sem) sys_open_sem(MUTEX_SEM_ID);

    for (uint64_t i = 0; i < loops; i++) {
        if (use_sem) sys_sem_wait(MUTEX_SEM_ID);
        int64_t tmp = global_balance;
        sys_yield();
        global_balance = tmp - 1;
        if (use_sem) sys_sem_post(MUTEX_SEM_ID);
    }
    sys_exit();
}

// test_sync: crea N pares inc/dec sobre la variable compartida. con sem el
// resultado final tiene que ser 0; sin sem se ve la corrupcion por races.
// argv: <pares> <loops> <use_sem (1|0)>
void test_sync(int argc, char ** argv) {
    if (argc != 3) {
        printf("test_sync: uso -> test_sync <pares> <loops> <use_sem 1|0>\n");
        sys_exit();
    }

    uint64_t pairs = satoi(argv[0]);
    int use_sem = satoi(argv[2]);

    if (pairs == 0 || pairs > MAX_PAIRS) {
        printf("test_sync: pares tiene que estar entre 1 y %d\n", MAX_PAIRS);
        sys_exit();
    }

    global_balance = 0;
    if (use_sem) sys_create_sem(MUTEX_SEM_ID, 1, "mutex");

    char * hijo_argv[] = { argv[1], argv[2] };   // loops, use_sem
    int64_t pids[2 * MAX_PAIRS];
    int n = 0;

    for (uint64_t i = 0; i < pairs; i++) {
        int64_t pi = sys_create_process((void *) &inc_process, "inc", 2, hijo_argv, 0, 0);
        if (pi > 0) pids[n++] = pi;
        int64_t pd = sys_create_process((void *) &dec_process, "dec", 2, hijo_argv, 0, 0);
        if (pd > 0) pids[n++] = pd;
    }

    // espero a TODOS los hijos con waitpid (antes era un semaforo SYNC)
    for (int i = 0; i < n; i++)
        sys_waitpid(pids[i]);

    printf("test_sync: balance final = %d\n", (int) global_balance);

    if (use_sem) sys_delete_sem(MUTEX_SEM_ID);
    sys_exit();
}
