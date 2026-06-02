#include <stdint.h>
#include <test_util.h>
#include <memLib.h>
#include <usrio.h>      // printf
#include <syscallLib.h> // sys_exit

#define MAX_BLOCKS 128

// guardo cada pedido: la direccion que me dio malloc y cuanto pedi
typedef struct {
    void *   address;
    uint32_t size;
} mm_rq;

// test_mm corre como proceso de usuario. argv[0] = memoria maxima en bytes.
// ciclo infinito: pide bloques random hasta llenar, los marca, verifica que
// no se pisen, los libera y vuelve a empezar. solo imprime si algo falla.
void test_mm(int argc, char ** argv) {
    mm_rq mm_rqs[MAX_BLOCKS];
    uint8_t rq;
    uint32_t total;
    uint64_t max_memory;

    if (argc != 1 || (max_memory = satoi(argv[0])) <= 0) {
        printf("test_mm: uso -> test_mm <memoria_max_en_bytes>\n");
        sys_exit();
    }

    while (1) {
        rq = 0;
        total = 0;

        // pido bloques hasta llenar el cupo de memoria o de slots
        while (rq < MAX_BLOCKS && total < max_memory) {
            // tamaño random entre 1 y lo que queda libre
            mm_rqs[rq].size = GetUniform(max_memory - total - 1) + 1;
            mm_rqs[rq].address = malloc(mm_rqs[rq].size);

            if (mm_rqs[rq].address != 0) {
                total += mm_rqs[rq].size;
                rq++;
            }
        }

        // a cada bloque le escribo un patron = su indice i
        for (uint32_t i = 0; i < rq; i++) {
            if (mm_rqs[i].address != 0)
                memset(mm_rqs[i].address, i, mm_rqs[i].size);
        }

        // verifico que el patron siga intacto. si no, dos bloques se pisaron
        for (uint32_t i = 0; i < rq; i++) {
            if (mm_rqs[i].address != 0) {
                if (!memcheck(mm_rqs[i].address, i, mm_rqs[i].size)) {
                    printf("test_mm ERROR: bloque %d corrupto\n", i);
                    sys_exit();
                }
            }
        }

        // libero todo y vuelvo a arrancar
        for (uint32_t i = 0; i < rq; i++) {
            if (mm_rqs[i].address != 0)
                free(mm_rqs[i].address);
        }
    }
}
