#ifndef _KEYBOARDDRIVER_H_
#define _KEYBOARDDRIVER_H_

#include <lib.h>
#include <color.h>
#include <stdint.h>
#include <stdbool.h>
#include <syscalls.h>

// Función de ASM para leer el scancode raw del puerto del teclado
extern unsigned int getKeyCode();

char procesScanCode(unsigned int scancode);

void loadCharToBuffer(char c);

// Función que se llama desde el IRQ dispatcher
// Devuelve el char
void keyboard_handler(); 

// Función para que las aplicaciones/kernel obtengan un carácter del buffer
// Retorna 0 si el buffer está vacío.
char kbd_get_char();

#endif