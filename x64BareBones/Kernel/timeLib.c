#include <timeLib.h>
#include <interrupts.h>
#include <syscalls.h>

static unsigned long ticks = 0;

void timer_handler() {
	ticks++;
}

int ticks_elapsed() {
	return ticks;
}

int seconds_elapsed() {
	return ticks / 18;
}

uint64_t ms_elapsed() {
	return (ticks * 1000) / 18;
}

/*
	TODO: Antes lo habia puesto contando los tick y no funcionaba
	bien en el userland, pero si en el kernel. Parece que es
	porque no le cedia control
*/
void sleep(uint64_t milliseconds) {
	uint64_t target_ticks = (milliseconds * 18) / 1000;
	uint64_t start_ticks = ticks;


	// Habilito los interrupts
	_sti();
	while ((ticks - start_ticks) < target_ticks);
	// Deshabilito los interrupts


	// TODO: Si dejaba esto no me andaba el teclado
	// _cli();
}

uint16_t getSec(){
	return getSysSeconds();
}
uint16_t getMin(){
	return getSysMinutes();
}
uint16_t  getHour(){
	return getSysHours();
}
uint16_t getDay(){
	return getSysDayOfMonth();
}
uint16_t  getMonth(){
	return getSysMonth();
}
uint16_t getYear(){
	return getSysYear();
}

void getTime(dateTime *dt) {
		dt->sec = getSec();
		dt->min = getMin();
		dt->hour = getHour();
		dt->day = getDay();
		dt->month = getMonth();
		dt->year = getYear();
}

