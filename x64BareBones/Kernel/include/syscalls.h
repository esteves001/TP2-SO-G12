#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include <fonts.h>
#include <stdint.h>
#include <stdbool.h>
#include <videoDriver.h>
#include <keyboardDriver.h>
#include <soundDriver.h>
#include <timeLib.h>
#include <lib.h>
#include <color.h>

// Estructura para acceder a los registros guardados por pushState
// El orden tiene que coincidir con el de la macro pushState en interrupts.asm
typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} Registers_t;



void syscallDispatcher(Registers_t *regs);
void loadSnapshot(Registers_t *regs);
void resetCursorCoord(void);

#endif /* SYSCALLS_H_ */