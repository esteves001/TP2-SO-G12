#include <test_util.h>

// PRNG "multiply-with-carry" (Marsaglia). dos semillas que se van pisando.
// no es seguro ni nada, alcanza para tirar tamaños/valores variados.
static uint32_t m_z = 362436069;
static uint32_t m_w = 521288629;

uint32_t GetUint(void) {
    m_z = 36969 * (m_z & 0xFFFF) + (m_z >> 16);
    m_w = 18000 * (m_w & 0xFFFF) + (m_w >> 16);
    return (m_z << 16) + m_w;
}

// me devuelve un numero en [0, max)
uint32_t GetUniform(uint32_t max) {
    uint32_t u = GetUint();
    // escalo u (0..2^32-1) al rango pedido sin usar float
    return (((uint64_t) u) * max) >> 32;
}

// recorro byte a byte; si alguno no es 'value' devuelvo 0 (fallo)
uint8_t memcheck(void * start, uint8_t value, uint32_t size) {
    uint8_t * p = (uint8_t *) start;
    for (uint32_t i = 0; i < size; i++) {
        if (p[i] != value)
            return 0;
    }
    return 1;
}

// atoi basico: solo digitos, sin signo (el param de memoria es positivo)
int64_t satoi(char * str) {
    int64_t res = 0;
    if (str == 0)
        return 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9')
            return 0;       // si hay basura, lo trato como invalido
        res = res * 10 + (str[i] - '0');
    }
    return res;
}
