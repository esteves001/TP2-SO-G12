#include <memLib.h>
#include <syscallLib.h>

// malloc de userland: es solo un wrapper sobre la syscall 0x41.
// el kernel me devuelve el bloque, o NULL si no hay lugar.
void * malloc(uint64_t size) {
    return sys_malloc(size);
}

// free: idem, syscall 0x42. el kernel sabe cuantas paginas liberar
// porque al reservar guardo el tamaño en un header antes del puntero.
void free(void * ptr) {
    sys_free(ptr);
}
