#ifndef _SHELL_H_
#define _SHELL_H_

#include <usrio.h>
#include <stdint.h>
#include <timeLib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stringLib.h>
#include <pongisLib.h>
#include <videoLib.h>

typedef enum {HELP = 0, EXC_1, EXC_2, PONGISGOLF, ZOOM_IN, ZOOM_OUT, CLEAR, DATE, REGISTERS, BUSY_WAIT, BUSY_WAIT_KERNEL, EXIT} command_id;

void startShell();
void readInput();
command_id processInput(char* input);
void help();
void printDateTime();
void notACommand();
void getRegisters();
void exitShell();
void clear_screen();
void startPongis();
void zoom_in();
void zoom_out();
void exception_1();
void exception_2();
void busy_wait();
void busy_wait_kernel();

#endif