#include <stdint.h>

#include <time.h>
#include <keyboardDriver.h>
#include "sem.h"

static void int_21();

void irqDispatcher(uint64_t irq) {

	// TODO: Esto depues hay que pasarlo a un array
	// Nota: el case 0 (timer tick) ya no se maneja aca, _irq00Handler
	// llama directo a schedule_tick para hacer el context switch.

	switch (irq) {

		// Teclado
		case 1:
			int_21();
			break;
	}
	return;
}

void int_21(){
	keyboard_handler();
	//sem_post(16);
}
