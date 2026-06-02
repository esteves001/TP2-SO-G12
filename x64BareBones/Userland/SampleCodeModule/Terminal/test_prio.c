#include <stdint.h>
#include <test_util.h>
#include <usrio.h>
#include <syscallLib.h>

#define TOTAL_PROC 3

// worker del test_prio: incrementa SU PROPIA variable hasta target y avisa.
// el incremento es trabajo real (CPU), no es busy-wait sobre una condicion:
// es lo que hace que se note quien tiene mas prioridad (termina antes).
// volatile para que el compilador no se coma el loop.
static void prio_worker(int argc, char ** argv) {
    int idx = satoi(argv[0]);
    uint64_t target = satoi(argv[1]);

    volatile uint64_t count = 0;
    while (count < target)
        count++;

    printf("  [test_prio] proceso %d (pid %d) llego a %d\n",
           idx, (int) sys_getpid(), (int) count);
    sys_exit();
}

// lanza una tanda de 3 workers con el mismo target y los espera a todos.
// si prios != NULL, le setea a cada uno su prioridad despues de crearlo.
static void correr_tanda(char * target_str, uint64_t * prios) {
    char idx_str[3];
    char * w_argv[2];
    w_argv[1] = target_str;     // todos van al mismo target

    int64_t pids[TOTAL_PROC];

    for (int i = 0; i < TOTAL_PROC; i++) {
        idx_str[0] = '0' + i;
        idx_str[1] = '\0';
        w_argv[0] = idx_str;    // el kernel copia la string al crear, puedo reusar el buffer
        pids[i] = sys_create_process((void *) &prio_worker, "prio_w", 2, w_argv, 0, 0);
        if (pids[i] > 0 && prios != 0)
            sys_nice(pids[i], prios[i]);
    }

    // espero a que terminen los 3 (waitpid vuelve enseguida si ya murio)
    for (int i = 0; i < TOTAL_PROC; i++)
        if (pids[i] > 0)
            sys_waitpid(pids[i]);
}

// test_prio: 2 tandas de 3 procesos que cuentan hasta target (argv[0]).
// ronda 1 todos con la misma prio (default), ronda 2 con prios 1/3/5: ahi
// se ve que el de mayor prio termina primero.
void test_prio(int argc, char ** argv) {
    if (argc != 1 || satoi(argv[0]) <= 0) {
        printf("test_prio: uso -> test_prio <valor_target>\n");
        sys_exit();
    }

    printf("test_prio: ronda 1 (misma prioridad)\n");
    correr_tanda(argv[0], 0);

    printf("test_prio: ronda 2 (prioridades 1, 3, 5)\n");
    uint64_t prios[TOTAL_PROC] = {1, 3, 5};
    correr_tanda(argv[0], prios);

    printf("test_prio: fin\n");
    sys_exit();
}
