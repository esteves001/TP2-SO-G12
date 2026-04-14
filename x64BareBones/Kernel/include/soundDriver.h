#ifndef _SOUND_DRIVER_H_
#define _SOUND_DRIVER_H_

#include <stdint.h>
#include <timeLib.h>

extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);

void play_sound(uint32_t nFrequence);
void nosound();

void playSoundForDuration(uint32_t nFrequence, uint32_t duration);

void beep();
void game_thud_sound();
void play_boot_sound();

#endif