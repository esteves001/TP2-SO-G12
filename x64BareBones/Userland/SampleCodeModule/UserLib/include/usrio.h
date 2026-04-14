#ifndef _USRIO_H_
#define _USRIO_H_

#include <stdarg.h>
#include <syscallLib.h>

int scanf(const char *format, ...);
int printf(const char *format, ...);
int putchar(char c);
char getchar(void);
int puts(const char *s);

void to_lower(char * str);
uint64_t get_regist(uint64_t *registers);


#endif