#include <tests.h>
#include <usrio.h>
#include <stringLib.h>
#include <syscallLib.h>

volatile int64_t global_balance = 0;
#define MUTEX_SEM_ID 1
#define SYNC_SEM_ID 2

static int my_atoi(const char *str) {
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] >= '0' && str[i] <= '9') {
            res = res * 10 + str[i] - '0';
        }
    }
    return res;
}

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

void inc_process(int argc, char **argv) {
    int n = my_atoi(argv[0]);
    int use_sem = my_atoi(argv[1]);

    if (use_sem) sys_open_sem(MUTEX_SEM_ID);
    sys_open_sem(SYNC_SEM_ID);

    for (int i = 0; i < n; i++) {
        if (use_sem) sys_sem_wait(MUTEX_SEM_ID);
        
        // --- SECCIÓN CRÍTICA ---
        // Lo dividimos en lectura, ceder CPU, y escritura para forzar el error si no hay semáforo
        int64_t temp = global_balance;
        sys_yield(); // Cede la CPU intencionalmente
        global_balance = temp + 1;
        // -----------------------

        // Imprimimos el estado unas 5 veces por proceso para no inundar la pantalla
        if (i > 0 && i % (n / 5) == 0) {
            printf("[INC] Balance: %d\n", (int)global_balance);
        }

        if (use_sem) sys_sem_post(MUTEX_SEM_ID);
    }

    sys_sem_post(SYNC_SEM_ID);
    sys_exit();
}

// Proceso que decrementa
void dec_process(int argc, char **argv) {
    int n = my_atoi(argv[0]);
    int use_sem = my_atoi(argv[1]);

    if (use_sem) sys_open_sem(MUTEX_SEM_ID);
    sys_open_sem(SYNC_SEM_ID);

    for (int i = 0; i < n; i++) {
        if (use_sem) sys_sem_wait(MUTEX_SEM_ID);
        
        // --- SECCIÓN CRÍTICA ---
int64_t temp = global_balance;
        sys_yield();
        global_balance = temp - 1;
        // -----------------------

        // Imprimir siempre
        printf("[DEC] Balance: %d\n", (int)global_balance);

        if (use_sem) sys_sem_post(MUTEX_SEM_ID);
    }

    sys_sem_post(SYNC_SEM_ID);
    sys_exit();
}

#define MUTEX_SEM_ID 1
#define SYNC_SEM_ID 2

// (El resto del código de inc_process, dec_process y my_atoi va arriba de esto)

// Función principal del test llamada desde la Shell
int test_sync_main(int argc, char **argv) {
    // Verificamos tener la cantidad correcta de argumentos
    // Asumo que tu shell manda los argumentos directamente en argv[0], argv[1], argv[2]
    // Si tu shell manda el nombre del comando en argv[0], cambiá los índices a 1, 2 y 3 y argc < 4.
    if (argc < 3) {
        printf("Uso: test_sync <pares_de_procesos> <loops> <use_sem (1 o 0)>\n");
        return -1;
    }

    // Convertimos los strings a números
    uint64_t pairs = my_atoi(argv[0]);
    uint64_t loops = my_atoi(argv[1]);
    int use_sem = my_atoi(argv[2]);

    printf("\nIniciando test_sync: %d pares, %d loops, semaforos: %s\n", 
           (int)pairs, (int)loops, use_sem ? "ON" : "OFF");

    global_balance = 0; // Reseteamos la variable

    // Preparamos los argumentos como strings para sys_create_process
    char *args_para_hijos[] = {argv[1], argv[2]}; // Le pasamos loops y use_sem

    // 1. Crear Semáforos
    if (use_sem) {
        sys_create_sem(MUTEX_SEM_ID, 1, "mutex"); 
    }
    sys_create_sem(SYNC_SEM_ID, 0, "sync");

    // 2. Lanzar procesos
    for (uint64_t i = 0; i < pairs; i++) {
        sys_create_process(&inc_process, "inc_proc", 2, args_para_hijos);
        sys_create_process(&dec_process, "dec_proc", 2, args_para_hijos);
    }

    // 3. Esperar a que todos terminen (Barrera de sincronización)
    for (uint64_t i = 0; i < pairs * 2; i++) {
        sys_sem_wait(SYNC_SEM_ID);
    }

    // 4. Imprimir resultado final
    printf("\nTest finalizado. Balance global final: %d\n", (int)global_balance);

    // 5. Limpieza
    if (use_sem) {
        sys_delete_sem(MUTEX_SEM_ID);
    }
    sys_delete_sem(SYNC_SEM_ID);

    return 0;
}