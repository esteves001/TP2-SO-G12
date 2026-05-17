GLOBAL sys_clearScreen
GLOBAL sys_putPixel
GLOBAL sys_drawChar
GLOBAL sys_drawString
GLOBAL sys_drawRectangle
GLOBAL sys_drawCircle
GLOBAL sys_drawDecimal
GLOBAL sys_drawHexa
GLOBAL sys_drawBin
GLOBAL sys_getScreenWidth
GLOBAL sys_getScreenHeight

GLOBAL sys_kbdGetChar

GLOBAL sys_getpid
GLOBAL sys_exit
GLOBAL sys_yield
GLOBAL sys_kill
GLOBAL sys_block
GLOBAL sys_unblock
GLOBAL sys_ps
GLOBAL sys_nice
GLOBAL sys_create_process
GLOBAL sys_mem_stats

GLOBAL sys_write
GLOBAL sys_read

GLOBAL sys_getRegisters
GLOBAL sys_getDateTime

GLOBAL sys_zoomIn
GLOBAL sys_zoomOut

GLOBAL sys_sleepMilli

GLOBAL opCodeException

section .text

%macro syscall 1
    mov rax, %1
    int 80h
    ret
%endmacro

sys_write:              syscall 0x1
sys_read:               syscall 0x2

sys_getRegisters:       syscall 0x04
sys_getDateTime:        syscall 0x05

sys_zoomIn:             syscall 0x06
sys_zoomOut:            syscall 0x07

; Video
sys_clearScreen:        syscall 0x10
sys_putPixel:           syscall 0x11
sys_drawChar:           syscall 0x12
sys_drawString:         syscall 0x13
sys_drawRectangle:      syscall 0x14
sys_drawDecimal:        syscall 0x15
sys_drawHexa:           syscall 0x16
sys_drawBin:            syscall 0x17
sys_getScreenWidth:     syscall 0x18
sys_getScreenHeight:    syscall 0x19

; Teclado
sys_kbdGetChar:         syscall 0x20

; Video (continuacion)
sys_drawCircle:         syscall 0x21

; Procesos
; Cada uno mete su numero en RAX, hace int 80h y vuelve. Los args ya vienen
; en RDI/RSI/... por la convencion de System V que usa GCC, asi que no toco nada.
; El kernel los lee en el dispatcher (Kernel/syscalls.c).
sys_getpid:             syscall 0x22
sys_exit:               syscall 0x23
sys_yield:              syscall 0x24
sys_kill:               syscall 0x25
sys_block:              syscall 0x26
sys_unblock:            syscall 0x27
sys_ps:                 syscall 0x28

; Memoria
sys_mem_stats:          syscall 0x29

; Prioridades
sys_nice:               syscall 0x30

; Procesos (continuacion)
sys_create_process:     syscall 0x31

; Tiempo
sys_sleepMilli:         syscall 0x40

opCodeException:
	ud2
	ret