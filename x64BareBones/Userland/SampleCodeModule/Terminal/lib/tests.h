#ifndef _TESTS_H_
#define _TESTS_H_

#include <stdint.h>

// test_prio: crea 3 procesos que se incrementan hasta N, con prios distintas.
// El de mayor prio deberia terminar antes (la marca '*' aparece primero).
// No espera a los hijos (no tenemos waitpid todavia), solo los lanza y sale.
int test_prio_main(int argc, char ** argv);

#endif
