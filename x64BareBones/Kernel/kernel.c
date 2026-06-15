#include <lib.h>
#include <stdint.h>
#include <string.h>
#include <fonts.h>
#include <videoDriver.h>   // para drawChar en el test del scheduler
#include <keyboardDriver.h>
#include <moduleLoader.h>
#include <idtLoader.h>
#include <timeLib.h>
#include <process.h>
#include <memoryManager.h>  // initialize_memory_manager
#include <interrupts.h>     // para force_schedule() en test_prio
#include "sem.h"

// Si descomento esto, en vez de levantar el userland normal arranco
// dos procesos de juguete (test_a y test_b) para ver si el context
// switch realmente conmuta. Util para debug del scheduler.
//#define SCHED_TEST

// Si descomento esto, lanzo un proceso de prueba que recibe args y los
// dibuja en pantalla para verificar que argc/argv llegan via RDI/RSI.
// #define ARGS_TEST

// Si descomento esto, lanzo 3 procesos que se incrementan hasta N, cada uno
// con prioridad distinta (1, 3, 5). Si el scheduler con prio funciona, los
// de mas prio terminan antes. Valida el scheduler con prioridades.
// #define PRIO_TEST

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void * const sampleCodeModuleAddress = (void*)0x400000;
static void * const sampleDataModuleAddress = (void*)0x500000;

typedef int (*EntryPoint)();


void clearBSS(void * bssAddress, uint64_t bssSize)
{
	memset(bssAddress, 0, bssSize);
}

void * getStackBase()
{
	return (void*)(
		(uint64_t)&endOfKernel
		+ PageSize * 8				//The size of the stack itself, 32KiB
		- sizeof(uint64_t)			//Begin at the top of the stack
	);
}

void * initializeKernelBinary()
{
	void * moduleAddresses[] = {
		sampleCodeModuleAddress,
		sampleDataModuleAddress
	};

	loadModules(&endOfKernelBinary, moduleAddresses);
	clearBSS(&bss, &endOfKernel - &bss);

	return getStackBase();
}

// Procesos de juguete para chequear que el scheduler conmuta entre procesos.
// Si anda bien tendrian que ir creciendo dos lineas en paralelo, una roja con
// As y otra verde con Bs. Si solo crece una, el switch no esta funcionando.
// El __attribute__((unused)) es para que el compilador no me tire warning
// cuando SCHED_TEST no esta definido (en ese caso estas funciones no se usan).
__attribute__((unused)) static void test_a() {
	uint64_t x = 0;
	while(1) {
		drawChar('A', 0xFF0000, x, 100);   // linea y=100, rojo
		x = (x + 10) % 800;
		for(volatile int i = 0; i < 1000000; i++); // delay para ver intercalado mas claro
	}
}

__attribute__((unused)) static void test_b() {
	uint64_t x = 0;
	while(1) {
		drawChar('B', 0x00FF00, x, 150);   // linea y=150, verde
		x = (x + 10) % 800;
		for(volatile int i = 0; i < 1000000; i++);
	}
}

// proceso de prueba para argc/argv. Dibuja argc como digito y cada argv[i]
// como string en lineas separadas. Si veo lo que pase desde main -> anduvo.
__attribute__((unused)) static void test_args(int argc, char ** argv) {
	drawChar('0' + (argc % 10), 0xFFFFFF, 0, 200);
	for(int i = 0; i < argc; i++) {
		int x = 0;
		for(int j = 0; argv[i][j] != 0; j++) {
			drawChar(argv[i][j], 0xFFFF00, x, 220 + i * 20);
			x += 10;
		}
	}
	while(1); // ya hizo su parte, halt
}

// === test_prio ===============================================================
#define PRIO_TEST_N 800  // hasta donde cuenta cada worker

static volatile uint64_t prio_counters[3] = {0};
static const char     prio_letters[3] = {'A', 'B', 'C'};
static const uint32_t prio_colors[3]  = {0xFF0000, 0x00FF00, 0x00BBFF};
static const uint64_t prio_ys[3]      = {300, 320, 340};

// worker del test_prio. Me identifico por mi pid: idle es 1, los 3 workers
// son pid 2/3/4 -> indice = pid-2. Incremento mi counter hasta N dibujando
// mi letra a lo largo de mi linea. Al terminar dibujo '*' y me auto-mato.
__attribute__((unused)) static void prio_worker() {
	int idx = (int)(current_process->pid - 2);
	if(idx < 0 || idx > 2) idx = 0; // safety

	while(prio_counters[idx] < PRIO_TEST_N) {
		prio_counters[idx]++;
		// dibujo cada 10 iters para no saturar la pantalla, x rota mod 800
		if(prio_counters[idx] % 10 == 0) {
			uint64_t x = ((prio_counters[idx] / 10) * 8) % 800;
			drawChar(prio_letters[idx], prio_colors[idx], x, prio_ys[idx]);
		}
		// delay para que cada iter tome tiempo real, sino terminan tan rapido
		// que ni alcanza a haber timer ticks entre medio. volatile para que
		// gcc no me optimice el loop.
		for(volatile int k = 0; k < 10000000; k++);
	}
	drawChar('*', 0xFFFFFF, 780, prio_ys[idx]); // marca de fin

	// me autoelimo: marco KILLED y fuerzo switch para no seguir corriendo
	// en stack muerto. El scheduler libera la pagina en la proxima pasada.
	exit_process(current_process);
	force_schedule();
	while(1); // no deberia volver
}

int main()
{
	load_idt();  // deja interrupciones apagadas, las prende el iretq de start_first_process
	initialize_memory_manager();  // sin esto, allocate_block devuelve NULL y no se crea ningun proceso

	create_idle_process();                                    // proceso 1: idle

#ifdef SCHED_TEST
	// Modo test del scheduler: dos procesos imprimiendo en pantalla
	create_process(&test_a, "test_a", 0, NULL, 0, 0);
	create_process(&test_b, "test_b", 0, NULL, 0, 0);
#elif defined(ARGS_TEST)
	// Modo test de argc/argv: arranca test_args con 3 strings
	char * args[] = {"hola", "mundo", "tp2"};
	create_process(&test_args, "test_args", 3, args, 0, 0);
#elif defined(PRIO_TEST)
	// 3 workers + prioridades 1, 3, 5. Los pids quedan 2, 3, 4 porque
	// idle ya se llevo el 1.
	create_process(&prio_worker, "p_a", 0, NULL, 0, 0);
	create_process(&prio_worker, "p_b", 0, NULL, 0, 0);
	create_process(&prio_worker, "p_c", 0, NULL, 0, 0);
	set_priority(2, 1);
	set_priority(3, 3);
	set_priority(4, 5);
#else
	create_process((void *)sampleCodeModuleAddress, "init", 0, NULL, 0, 0);
#endif

	scheduler();                                              // elige el primero (queda en current_process)
	start_first_process(current_process->rsp);                // salta al primero, no vuelve

	return 0; // nunca llega
}
