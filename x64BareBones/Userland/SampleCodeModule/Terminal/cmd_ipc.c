#include <cmd_ipc.h>
#include <usrio.h>
#include <syscallLib.h>
#include <test_util.h>
#include <stringLib.h>   // strcmp

#define STDIN  0
#define STDOUT 1

#define MAX_PS_BUF 16    // = MAX_PROCESSES, lo que entra en el snapshot de ps

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

/* un color por lector = el "identificador unico" que pide el enunciado.
   el indice 0 es el "rojo" (el que el enunciado mata en uno de los casos). */
static const uint32_t reader_colors[MAX_RD] = {
    0xFF0000, /* rojo    */
    0x00FF00, /* verde   */
    0x00BFFF, /* celeste */
    0xFFFF00, /* amarillo*/
    0xFF00FF, /* magenta */
    0xFF8000, /* naranja */
    0x4060FF, /* azul    */
    0xFFFFFF, /* blanco  */
};

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
    int idx = argv[0][0] - '0';              // mi numero de lector (0,1,2...)
    if (idx < 0 || idx >= MAX_RD) idx = 0;
    sys_set_text_color(reader_colors[idx]);  // de aca en mas imprimo en mi color

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

/* limpia una corrida anterior de mvar: mata los lectores/escritores viejos y
   borra los semaforos. asi puedo correr mvar de nuevo sin rebootear, sin
   arrastrar procesos zombie ni semaforos con pids fantasma de un kill previo. */
static void mvar_cleanup(void) {
    process_info_t buf[MAX_PS_BUF];
    int n = sys_ps(buf, MAX_PS_BUF);
    for (int i = 0; i < n; i++) {
        if (strcmp(buf[i].name, "mvar_wr") == 0 || strcmp(buf[i].name, "mvar_rd") == 0)
            sys_kill(buf[i].pid);
    }
    // delete es no-op si el sem no existe, asi que es seguro llamarlo siempre
    sys_delete_sem(MVAR_EMPTY);
    sys_delete_sem(MVAR_FULL);
    sys_delete_sem(MVAR_MUTEX);
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

    mvar_cleanup();   // limpio cualquier corrida anterior

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