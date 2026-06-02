#include <stdint.h>
#include <test_util.h>
#include <usrio.h>
#include <syscallLib.h>

#define TEST_MAX_PROC 16   // tope del kernel (MAX_PROCESSES)

// estados que YO le llevo a cada dummy (no son los del kernel)
enum p_state { ST_RUNNING, ST_BLOCKED, ST_KILLED };

typedef struct {
    int64_t      pid;
    enum p_state state;
} p_rq;

// proceso dummy: no hace nada util, solo existe para que el test lo
// bloquee/desbloquee/mate. cede la CPU para no quemar ciclos al pedo.
void endless_loop(int argc, char ** argv) {
    while (1)
        sys_yield();
}

// test_proc: crea N dummies y al azar los mata/bloquea/desbloquea hasta
// matarlos a todos; despues vuelve a crearlos. solo imprime si algo falla.
void test_proc(int argc, char ** argv) {
    p_rq p_rqs[TEST_MAX_PROC];
    uint8_t rq;
    uint8_t alive;
    uint8_t action;
    uint64_t max_processes;
    char * argv_dummy[] = { 0 };   // los dummies no reciben args

    if (argc != 1 || (max_processes = satoi(argv[0])) <= 0
            || max_processes > TEST_MAX_PROC) {
        printf("test_proc: uso -> test_proc <cant_procesos (1..%d)>\n", TEST_MAX_PROC);
        sys_exit();
    }

    while (1) {
        alive = 0;

        // creo los dummies
        for (rq = 0; rq < max_processes; rq++) {
            p_rqs[rq].pid = sys_create_process((void *)&endless_loop, "endless", 0, argv_dummy);

            if (p_rqs[rq].pid == -1) {
                printf("test_proc ERROR: no se pudo crear el proceso\n");
                sys_exit();
            }
            p_rqs[rq].state = ST_RUNNING;
            alive++;
        }

        // hasta matarlos a todos: mato o bloqueo al azar, y desbloqueo al azar
        while (alive > 0) {
            for (rq = 0; rq < max_processes; rq++) {
                action = GetUniform(100) % 2;

                if (action == 0) {
                    // mato (si sigue vivo: corriendo o bloqueado)
                    if (p_rqs[rq].state == ST_RUNNING || p_rqs[rq].state == ST_BLOCKED) {
                        if (sys_kill(p_rqs[rq].pid) == -1) {
                            printf("test_proc ERROR: no se pudo matar el proceso\n");
                            sys_exit();
                        }
                        p_rqs[rq].state = ST_KILLED;
                        alive--;
                    }
                } else {
                    // bloqueo (solo si esta corriendo)
                    if (p_rqs[rq].state == ST_RUNNING) {
                        if (sys_block(p_rqs[rq].pid) == -1) {
                            printf("test_proc ERROR: no se pudo bloquear el proceso\n");
                            sys_exit();
                        }
                        p_rqs[rq].state = ST_BLOCKED;
                    }
                }
            }

            // desbloqueo al azar a los que quedaron bloqueados
            for (rq = 0; rq < max_processes; rq++) {
                if (p_rqs[rq].state == ST_BLOCKED && (GetUniform(100) % 2)) {
                    if (sys_unblock(p_rqs[rq].pid) == -1) {
                        printf("test_proc ERROR: no se pudo desbloquear el proceso\n");
                        sys_exit();
                    }
                    p_rqs[rq].state = ST_RUNNING;
                }
            }
        }
    }
}
