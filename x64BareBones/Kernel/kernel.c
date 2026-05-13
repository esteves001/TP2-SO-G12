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

// Si descomento esto, en vez de levantar el userland normal arranco
// dos procesos de juguete (test_a y test_b) para ver si el context
// switch realmente conmuta. Util para debug del scheduler.
// #define SCHED_TEST

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
		for(volatile int i = 0; i < 10000000; i++); // delay choto para que no se llene todo en un tick
	}
}

__attribute__((unused)) static void test_b() {
	uint64_t x = 0;
	while(1) {
		drawChar('B', 0x00FF00, x, 150);   // linea y=150, verde
		x = (x + 10) % 800;
		for(volatile int i = 0; i < 10000000; i++);
	}
}

int main()
{
	load_idt();  // deja interrupciones apagadas, las prende el iretq de start_first_process
	initialize_memory_manager();  // sin esto, allocate_page devuelve NULL y no se crea ningun proceso

	create_idle_process();                                    // proceso 1: idle

#ifdef SCHED_TEST
	// Modo test del scheduler: dos procesos imprimiendo en pantalla
	create_process(&test_a, "test_a");
	create_process(&test_b, "test_b");
#else
	create_process((void *)sampleCodeModuleAddress, "init");  // proceso 2: userland normal
#endif

	scheduler();                                              // elige el primero (queda en current_process)
	start_first_process(current_process->rsp);                // salta al primero, no vuelve

	return 0; // nunca llega
}
