GLOBAL cpuVendor
GLOBAL getSysSeconds
GLOBAL getSysMinutes
GLOBAL getSysHours
GLOBAL getSysDayOfWeek
GLOBAL getSysDayOfMonth
GLOBAL getSysMonth
GLOBAL getSysYear

GLOBAL getKeyCode

; Lo usamos para sonido nosotros
GLOBAL outb
GLOBAL inb

section .text
	
cpuVendor:
	push rbp
	mov rbp, rsp

	push rbx

	mov rax, 0
	cpuid


	mov [rdi], ebx
	mov [rdi + 4], edx
	mov [rdi + 8], ecx

	mov byte [rdi+13], 0

	mov rax, rdi

	pop rbx

	mov rsp, rbp
	pop rbp
	ret


;Funcionalidades del RTC

; Recibe en al el numero y lo devuelve
; en al pasado a decimal.
; Si por ejemplo la entrada es
; 0100 0101, los primeros 4 bits
; representan un 4 y los otros un 5,
; asi que devolvemos 45
convertBCDtoDec:
	push rbp			; Armado del stackframe
	mov rbp, rsp		;

	push rbx			; Back up de los registros
	push rcx			;
	push rdx			;

	mov rbx, 0			; Limpio registros
	mov rcx, 0			;

	mov bl, al			; Backup del original

	and al, 0x0F		; Agarro los 4 lsb
	mov cl, al			; los guardo en cl

	mov al, bl			; Agarro el numero original
	shr al, 4			; Muevo los 4 msb a la derecha

	mov bl, 10
	mul bl				; Multiplica por 10 al

	add al, cl			; Le agrega lo calculado antes		

	pop rdx
	pop rcx
	pop rbx

	mov rsp, rbp
	pop rbp 
	ret

getSysSeconds:
	push rbp
	mov rbp, rsp

	mov al, 0

	out 70h, al
	in al, 71h

	call convertBCDtoDec

	mov rsp, rbp
	pop rbp
	ret


getSysMinutes:
	push rbp
	mov rbp, rsp

	mov al, 2

	out 70h, al
	in al, 71h

	call convertBCDtoDec

	mov rsp, rbp
	pop rbp
	ret

getSysHours:
	push rbp
	mov rbp, rsp

	mov al, 4

	out 70h, al
	in al, 71h

	call convertBCDtoDec

	mov rsp, rbp
	pop rbp
	ret

getSysDayOfWeek:
	push rbp
	mov rbp, rsp

	mov al, 6

	out 70h, al
	in al, 71h

	call convertBCDtoDec 

	mov rsp, rbp
	pop rbp
	ret

getSysDayOfMonth:
	push rbp
	mov rbp, rsp

	mov al, 7

	out 70h, al
	in al, 71h

	call convertBCDtoDec 

	mov rsp, rbp
	pop rbp
	ret

getSysMonth:
	push rbp
	mov rbp, rsp

	mov al, 8

	out 70h, al
	in al, 71h

	call convertBCDtoDec 

	mov rsp, rbp
	pop rbp
	ret

getSysYear:
	push rbp
	mov rbp, rsp

	mov al, 9

	out 70h, al
	in al, 71h

	call convertBCDtoDec 

	mov rsp, rbp
	pop rbp
	ret


getKeyCode:
	push rbp
	mov rbp, rsp
	
	mov rax, 0      ; Valor de retorno por defecto: 0 (ningún carácter / no imprimible)

	in al, 64h		; Leer el puerto de estado del controlador del teclado (flags)
	test al, 0x01	; Comprobar el bit 0 (Output Buffer Full - OBF)
	jz .exit_gk		; Si no está lleno (no hay tecla), saltar al final (RAX ya es 0)

					; Una tecla está disponible, leerla
	in al, 60h 		; Leer el scancode del puerto de datos del teclado

.exit_gk:
	mov rsp, rbp
	pop rbp
	ret

outb:
    mov dx, di
    mov al, sil 
    out dx, al
    ret

inb:
    mov dx, di 
    in al, dx
    movzx rax, al
    ret