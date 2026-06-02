#ifndef _TESTS_H_
#define _TESTS_H_

#include <stdint.h>

// entry points de los tests. corren como procesos de usuario (lanzar con
// sys_create_process). cada uno valida sus args y sale con sys_exit().
void test_mm(int argc, char ** argv);     // <memoria_max_bytes>
void test_proc(int argc, char ** argv);   // <cant_procesos>
void test_prio(int argc, char ** argv);   // <valor_target>
void test_sync(int argc, char ** argv);   // <pares> <loops> <use_sem>

// demo de pipe (no es del enunciado), sigue corriendo inline desde la shell.
int test_pipe_main(int argc, char ** argv);
#endif
