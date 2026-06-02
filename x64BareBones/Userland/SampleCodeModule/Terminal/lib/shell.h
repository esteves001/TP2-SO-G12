#ifndef _SHELL_H_
#define _SHELL_H_

#include <usrio.h>
#include <stdint.h>
#include <timeLib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stringLib.h>
#include <videoLib.h>
#include <syscallLib.h>
#include <tests.h>

void startShell(void);
void readInput(char *buffer);
void notACommand(char *input);
void exitShell(void);
void show_prompt(void);

#endif
