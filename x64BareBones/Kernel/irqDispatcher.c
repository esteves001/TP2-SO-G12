#include <stdint.h>

#include <time.h>
#include <keyboardDriver.h>

static void int_20();
static void int_21();

void irqDispatcher(uint64_t irq) {

	// TODO: Esto depues hay que pasarlo a un array

	switch (irq) {

		// Timer Tick
		case 0:
			int_20();
			break;

		// Teclado
		case 1:
			int_21();
			break;
	}
	return;
}

void int_20() {
	timer_handler();
}

void int_21(){
	keyboard_handler();
}
