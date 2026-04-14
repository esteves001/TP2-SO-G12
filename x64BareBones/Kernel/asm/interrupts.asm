GLOBAL _cli
GLOBAL _sti
GLOBAL picMasterMask
GLOBAL picSlaveMask
GLOBAL haltcpu
GLOBAL _hlt

GLOBAL _irq00Handler
GLOBAL _irq01Handler
GLOBAL _irq02Handler
GLOBAL _irq03Handler
GLOBAL _irq04Handler
GLOBAL _irq05Handler

GLOBAL _exception0Handler
GLOBAL _exception6Handler
GLOBAL _int80Handler

GLOBAL get_registers

EXTERN irqDispatcher
EXTERN exceptionDispatcher
EXTERN syscallDispatcher
EXTERN keyboard_handler
EXTERN getStackBase

section .rodata
    userland equ 0x400000

SECTION .text

%macro pushState 0
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro popState 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax
%endmacro

%macro irqHandlerMaster 1
	pushState

	mov rdi, %1
	call irqDispatcher

	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al

	popState
	iretq
%endmacro


%macro exceptionHandler 1
    pushState

    ; Pasamos un puntero a los registros guardados (RSP) y el numero de excepcion
    ; a nuestro manejador en C.
    mov rdi, rsp
    mov rsi, %1
    call exceptionDispatcher

    ; Ahora modificamos el RIP guardado en la pila para que, al volver,
    ; la ejecucion se reanude al principio del modulo de Userland (la shell).
    ; La CPU guarda 5 registros (SS, RSP, RFLAGS, CS, RIP) en una interrupcion.
    ; Nuestra macro pushState guarda 15 registros.
    ; RIP está en [rsp + 15*8] = [rsp + 120] desde donde estaba RSP antes de esta llamada.
    mov qword [rsp + 120], userland

    popState

    ; Las excepciones 0 (Div by Zero) y 6 (Invalid Opcode) no pushean un 
    ; error code, asi que solamente hacemos iretq para volver
    iretq
%endmacro


_hlt:
	sti
	hlt
	ret

_cli:
	cli
	ret


_sti:
	sti
	ret

picMasterMask:
	push rbp
    mov rbp, rsp
    mov ax, di
    out	21h,al
    pop rbp
    retn

picSlaveMask:
	push    rbp
    mov     rbp, rsp
    mov     ax, di  ; ax = mascara de 16 bits
    out	0A1h,al
    pop     rbp
    retn


;8254 Timer (Timer Tick)
_irq00Handler:
	irqHandlerMaster 0

;Keyboard
_irq01Handler:
    pushState

	; En vez de llamar a irqMaster lo hacemos
	; asi porque el otro ya tiene un ireq, 
	; asi que toda la parte depues la llamada
	; no se ejecutaria. Lo dejamos asi para que
	; Este irtq funcione
	mov rdi, rsp
	call keyboard_handler

	; signal pic EOI (End of Interrupt)
	; Esto es por lo de no poder usar irqMaster
	mov al, 20h
	out 20h, al

    popState
    iretq

;Cascade pic never called
_irq02Handler:
	irqHandlerMaster 2

;Serial Port 2 and 4
_irq03Handler:
	irqHandlerMaster 3

;Serial Port 1 and 3
_irq04Handler:
	irqHandlerMaster 4

;USB
_irq05Handler:
	irqHandlerMaster 5


;Zero Division Exception
_exception0Handler:
	exceptionHandler 0

;Invalid Operation Code
_exception6Handler: 
	exceptionHandler 6

; Manejador para la interrupcion 0x80h (syscalls)
_int80Handler:
    pushState

    mov rdi, rsp
    call syscallDispatcher

    ; El valor de retorno de la syscall (si lo hay) deberia estar en rax
    ; despues de que syscallDispatcher retorne. popState lo restaura

    popState
    iretq


haltcpu:
	cli
	hlt
	ret

; Esto al final por la forma en la que hacemos
; lo del teclado no la usamos por ahora
get_registers:
	pushState
    mov rax, rsp
	popState
    ret


SECTION .bss
	user_snapshot resq 17
