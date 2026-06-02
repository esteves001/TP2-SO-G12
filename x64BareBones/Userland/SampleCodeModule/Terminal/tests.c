#include <tests.h>
#include <usrio.h>
#include <stringLib.h>
#include <syscallLib.h>
#include <stddef.h>

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
