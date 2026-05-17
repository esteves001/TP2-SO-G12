#include <tests.h>
#include <syscallLib.h>

// worker del test_prio en userland. Recibe en argv[0] su indice ("0"/"1"/"2"),
// que usa para elegir letra, color y linea Y propios. Loopea hasta N
// dibujando con sys_drawChar, y al terminar marca '*' y hace sys_exit.
static void prio_worker_user(int argc, char ** argv) {
    if(argc < 1) sys_exit();
    int idx = argv[0][0] - '0';
    if(idx < 0 || idx > 2) sys_exit();

    static const char     letters[3] = {'A', 'B', 'C'};
    static const uint32_t colors[3]  = {0xFF0000, 0x00FF00, 0x00BBFF};
    static const uint64_t ys[3]      = {300, 320, 340};

    const int N = 800;
    for(int i = 1; i <= N; i++) {
        if(i % 10 == 0) {
            uint64_t x = ((i / 10) * 8) % 800;
            sys_drawChar(letters[idx], colors[idx], x, ys[idx]);
        }
        // delay para que cada iter tome tiempo real, sino terminan antes
        // que haya un context switch y no se ve el efecto de prios.
        for(volatile int k = 0; k < 5000000; k++);
    }
    sys_drawChar('*', 0xFFFFFF, 780, ys[idx]);
    sys_exit();
}

int test_prio_main(int argc, char ** argv) {
    // creo los 3 workers pasandoles su indice como string en argv[0]
    char * a_args[] = {"0"};
    char * b_args[] = {"1"};
    char * c_args[] = {"2"};

    int64_t pid_a = sys_create_process((void *)&prio_worker_user, "p_a", 1, a_args);
    int64_t pid_b = sys_create_process((void *)&prio_worker_user, "p_b", 1, b_args);
    int64_t pid_c = sys_create_process((void *)&prio_worker_user, "p_c", 1, c_args);

    // les seteo prios distintas: 1, 3, 5. El de prio 5 deberia terminar
    // primero (azul), despues el de 3 (verde), ultimo el de 1 (rojo).
    if(pid_a > 0) sys_nice(pid_a, 1);
    if(pid_b > 0) sys_nice(pid_b, 3);
    if(pid_c > 0) sys_nice(pid_c, 5);

    return 0;
}
