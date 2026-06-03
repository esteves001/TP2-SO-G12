#include <tests.h>
#include <usrio.h>
#include <stringLib.h>
#include <syscallLib.h>
#include <stddef.h>

volatile int64_t global_balance = 0;
#define MUTEX_SEM_ID 1
#define SYNC_SEM_ID 2

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

    int64_t pid_a = sys_create_process((void *)&prio_worker_user, "p_a", 1, a_args, 0, 0);
    int64_t pid_b = sys_create_process((void *)&prio_worker_user, "p_b", 1, b_args, 0, 0);
    int64_t pid_c = sys_create_process((void *)&prio_worker_user, "p_c", 1, c_args, 0, 0);

    // les seteo prios distintas: 1, 3, 5. El de prio 5 deberia terminar
    // primero (azul), despues el de 3 (verde), ultimo el de 1 (rojo).
    if(pid_a > 0) sys_nice(pid_a, 1);
    if(pid_b > 0) sys_nice(pid_b, 3);
    if(pid_c > 0) sys_nice(pid_c, 5);

    return 0;
}


void inc_process(int argc, char **argv) {
    int n = atoi(argv[0]);
    int use_sem = atoi(argv[1]);

    if (use_sem) sys_open_sem(MUTEX_SEM_ID);
    sys_open_sem(SYNC_SEM_ID);

    // FIX 1: Calculamos un divisor seguro para no disparar excepcion por division por cero
    int step = (n >= 5) ? (n / 5) : 1;

    for (int i = 0; i < n; i++) {
        if (use_sem) sys_sem_wait(MUTEX_SEM_ID);
        
        // --- SECCIÓN CRÍTICA ---
        int64_t temp = global_balance;
        sys_yield(); // Cede la CPU intencionalmente
        global_balance = temp + 1;
        // -----------------------

        // Imprimimos el estado unas 5 veces por proceso para no inundar la pantalla
        if (i > 0 && i % step == 0) {
            printf("[INC] Balance: %d\n", (int)global_balance);
        }

        if (use_sem) sys_sem_post(MUTEX_SEM_ID);
    }

    sys_sem_post(SYNC_SEM_ID);
    sys_exit();
}

// Proceso que decrementa
void dec_process(int argc, char **argv) {
    int n = atoi(argv[0]);
    int use_sem = atoi(argv[1]);

    if (use_sem) sys_open_sem(MUTEX_SEM_ID);
    sys_open_sem(SYNC_SEM_ID);

    // FIX 1: Lo mismo acá
    int step = (n >= 5) ? (n / 5) : 1;

    for (int i = 0; i < n; i++) {
        if (use_sem) sys_sem_wait(MUTEX_SEM_ID);
        
        // --- SECCIÓN CRÍTICA ---
        int64_t temp = global_balance;
        sys_yield();
        global_balance = temp - 1;
        // -----------------------

        // Imprimir siempre usando el step seguro
        if (i > 0 && i % step == 0) {
            printf("[DEC] Balance: %d\n", (int)global_balance);
        }

        if (use_sem) sys_sem_post(MUTEX_SEM_ID);
    }

    sys_sem_post(SYNC_SEM_ID);
    sys_exit();
}

int test_sync_main(int argc, char **argv) {

    if (argc < 3) {
        printf("Uso: test_sync <pares_de_procesos> <loops> <use_sem (1 o 0)>\n");
        return -1;
    }

    uint64_t pairs = atoi(argv[0]);
    
    // --- VALIDACIÓN DE LÍMITE DE PROCESOS ---
    // Sabiendo que el SO soporta 16 procesos, y 2 ya pueden estar ocupados
    // por la Shell y este test_sync_main, limitamos a 7 pares (14 procesos).
    if (pairs > 7) {
        printf("Error: Superaste el limite de procesos del sistema.\n");
        printf("Tu Kernel soporta un maximo de 16 procesos.\n");
        printf("Por favor, volve a ejecutar el comando con un maximo de 7 pares.\n");
        return -1;
    }
    // ----------------------------------------

    uint64_t loops = atoi(argv[1]);
    int use_sem = atoi(argv[2]);

    printf("\nIniciando test_sync: %d pares, %d loops, semaforos: %s\n", 
           (int)pairs, (int)loops, use_sem ? "ON" : "OFF");

    global_balance = 0;

    char *args_para_hijos[] = {argv[1], argv[2]}; 

    if (use_sem) {
        sys_create_sem(MUTEX_SEM_ID, 1, "mutex"); 
    }
    
    sys_create_sem(SYNC_SEM_ID, 0, "sync");

    // Llevamos la cuenta real de los procesos que lograron crearse
    uint64_t procesos_creados = 0;

    for (uint64_t i = 0; i < pairs; i++) {
        if (sys_create_process(&inc_process, "inc_proc", 2, args_para_hijos, 0, 0) > 0) procesos_creados++;
        if (sys_create_process(&dec_process, "dec_proc", 2, args_para_hijos, 0, 0) > 0) procesos_creados++;
    }

    // Esperamos SOLAMENTE a la cantidad real de hijos que nacieron
    for (uint64_t i = 0; i < procesos_creados; i++) {
        sys_sem_wait(SYNC_SEM_ID);
    }

    printf("\nTest finalizado. Balance global final: %d\n", global_balance);

    if (use_sem) {
        sys_delete_sem(MUTEX_SEM_ID);
    }
    sys_delete_sem(SYNC_SEM_ID);

    return 0;
}

#define PIPE_TEST_ID 1   // id del pipe (tabla de pipes, no choca con los sems)
#define PIPE_SYNC_ID 5   // sem para esperar a los hijos (waitpid casero)

// escritor: abre el pipe por id y manda el mensaje en trozos, con un delay
// antes de cada uno para que el lector tenga que bloquearse esperando datos.
static void pipe_writer(int argc, char ** argv) {
    sys_open_pipe(PIPE_TEST_ID);
    sys_open_sem(PIPE_SYNC_ID);

    char * msgs[] = {"hola ", "desde ", "otro ", "proceso ", "via pipe!\n"};
    for(int i = 0; i < 5; i++) {
        sys_sleepMilli(700); // pausa real para que se note el bloqueo del lector
        sys_pipe_write(PIPE_TEST_ID, msgs[i], strlen(msgs[i]));
    }

    sys_sem_post(PIPE_SYNC_ID); // aviso que termine
    sys_exit();
}

// lector: abre el pipe y lee de a 1 byte. si esta vacio se bloquea (sin busy
// wait) hasta que el escritor mande algo. corta cuando llega el '\n'.
static void pipe_reader(int argc, char ** argv) {
    sys_open_pipe(PIPE_TEST_ID);
    sys_open_sem(PIPE_SYNC_ID);

    char c;
    do {
        putchar('\t'); // marco cada espera bloqueante del lector con un tab
        sys_pipe_read(PIPE_TEST_ID, &c, 1); // bloquea si el pipe esta vacio
        putchar(c);
    } while(c != '\n');

    sys_sem_post(PIPE_SYNC_ID);
    sys_exit();
}

// lanza un lector y un escritor NO emparentados que comparten el pipe por id.
// el lector arranca, se bloquea leyendo, y el escritor lo va despertando.
int test_pipe_main(int argc, char ** argv) {
    printf("\n--- Test de Pipe (productor/consumidor por id) ---\n");

    if(sys_create_pipe(PIPE_TEST_ID) < 0) {
        printf("No se pudo crear el pipe (id ocupado?)\n");
        return -1;
    }
    sys_create_sem(PIPE_SYNC_ID, 0, "pipe_sync");

    int creados = 0;
    if(sys_create_process(&pipe_reader, "p_reader", 0, NULL, 0, 0) > 0) creados++;
    if(sys_create_process(&pipe_writer, "p_writer", 0, NULL, 0, 0) > 0) creados++;

    // espero a que terminen los dos (waitpid casero con el sem)
    for(int i = 0; i < creados; i++) sys_sem_wait(PIPE_SYNC_ID);

    sys_close_pipe(PIPE_TEST_ID);
    sys_delete_sem(PIPE_SYNC_ID);
    printf("--- Test de pipe finalizado ---\n");
    return 0;
}
