#include <cmd_ipc.h>
#include <usrio.h>
#include <syscallLib.h>
#include <test_util.h>

#define STDIN  0
#define STDOUT 1

static int is_vowel(char c) {
    return c=='a'||c=='e'||c=='i'||c=='o'||c=='u'||
           c=='A'||c=='E'||c=='I'||c=='O'||c=='U';
}

void cmd_cat(int argc, char **argv) {
    char c;
    while (sys_read(STDIN, &c, 1) > 0)
        sys_write(STDOUT, &c, 1);
    sys_exit();
}

void cmd_wc(int argc, char **argv) {
    char c;
    int lines = 0;
    while (sys_read(STDIN, &c, 1) > 0)
        if (c == '\n') lines++;
    printf("%d\n", lines);
    sys_exit();
}

void cmd_filter(int argc, char **argv) {
    char c;
    while (sys_read(STDIN, &c, 1) > 0)
        if (is_vowel(c)) sys_write(STDOUT, &c, 1);
    sys_exit();
}

/* ================================================================== */
/*  mvar — multiples lectores/escritores sobre una variable global    */
/*  estilo MVar de Haskell. La MVar esta "vacia" o "llena".           */
/*  Escritor: espera vacia -> escribe su letra unica.                 */
/*  Lector : espera llena -> consume e imprime.                       */
/*  Solo un proceso accede a la vez (mutex). Sin busy-wait en la      */
/*  sincronizacion (los sem bloquean); el busy-wait random es el que  */
/*  pide el enunciado.                                                 */
/*  uso: mvar <escritores> <lectores>                                  */
/* ================================================================== */

#define MVAR_EMPTY  10   /* ids de semaforo acordados a priori */
#define MVAR_FULL   11
#define MVAR_MUTEX  12

#define MAX_WR 8
#define MAX_RD 8

/* variable global compartida (mismo espacio de memoria, sin MMU) */
static volatile char mvar_value = 0;

/* espera activa aleatoria, como pide el enunciado */
static void busy_wait_random(void) {
    uint32_t spins = GetUniform(5000000) + 1000000;
    for (volatile uint32_t i = 0; i < spins; i++);
}

/* escritor: argv[0] = letra unica ("A","B",...) */
static void mvar_writer(int argc, char **argv) {
    char letter = argv[0][0];
    sys_open_sem(MVAR_EMPTY);
    sys_open_sem(MVAR_FULL);
    sys_open_sem(MVAR_MUTEX);

    while (1) {
        busy_wait_random();
        sys_sem_wait(MVAR_EMPTY);   /* espera a que la MVar este vacia */
        sys_sem_wait(MVAR_MUTEX);

        mvar_value = letter;        /* escribe su valor */

        sys_sem_post(MVAR_MUTEX);
        sys_sem_post(MVAR_FULL);    /* avisa que hay un valor para leer */
    }
}

/* lector: consume el valor y lo imprime */
static void mvar_reader(int argc, char **argv) {
    sys_open_sem(MVAR_EMPTY);
    sys_open_sem(MVAR_FULL);
    sys_open_sem(MVAR_MUTEX);

    while (1) {
        busy_wait_random();
        sys_sem_wait(MVAR_FULL);    /* espera a que haya un valor */
        sys_sem_wait(MVAR_MUTEX);

        char c = mvar_value;        /* consume el valor */

        sys_sem_post(MVAR_MUTEX);
        sys_sem_post(MVAR_EMPTY);   /* avisa que la MVar quedo vacia */

        char str[2] = { c, 0 };
        printf("%s", str);
    }
}

void cmd_mvar(int argc, char **argv) {
    if (argc < 3) {
        printf("uso: mvar <escritores> <lectores>\n");
        sys_exit();
        return;
    }

    int writers = (int)satoi(argv[1]);
    int readers = (int)satoi(argv[2]);

    if (writers <= 0 || writers > MAX_WR || readers <= 0 || readers > MAX_RD) {
        printf("error: escritores y lectores deben ser 1..8\n");
        sys_exit();
        return;
    }

    /* arranca vacia: se puede escribir, no se puede leer */
    sys_create_sem(MVAR_EMPTY, 1, "mvar_empty");
    sys_create_sem(MVAR_FULL,  0, "mvar_full");
    sys_create_sem(MVAR_MUTEX, 1, "mvar_mutex");

    mvar_value = 0;

    for (int i = 0; i < writers; i++) {
        char letter[2] = { 'A' + i, 0 };
        char *args[1]  = { letter };
        sys_create_process((void *)mvar_writer, "mvar_wr", 1, args, 0, 0);
    }

    for (int i = 0; i < readers; i++) {
        char idx[2]   = { '0' + i, 0 };
        char *args[1] = { idx };
        sys_create_process((void *)mvar_reader, "mvar_rd", 1, args, 0, 0);
    }

    sys_exit();
}