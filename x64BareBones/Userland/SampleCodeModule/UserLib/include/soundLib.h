#ifndef SOUNDLIB_H
#define SOUNDLIB_H

#include <syscallLib.h>
#include <stdint.h>

void playSound(uint32_t frequency, uint32_t duration);
void playHitSound();
void playStartMusic();
void playHoleMusic();

#endif