#include <syscalls.h>
#include "memoryManager.h"
#include "process.h"
#include "interrupts.h"  // para force_schedule()

#define STDIN  0
#define STDOUT 1

#define VERDE   0x8bd450
#define VIOLETA 0x8b4bd4
#define BLANCO  0xffffff
#define NEGRO   0x000000

#define TAB_SPACES 4

// Coordinadas del cursor
static uint16_t x_coord = 0;
static uint16_t y_coord = 0;

Registers_t snapshot = { 0 };
bool snapshotReady = false;

void resetCursorCoord()
{
    x_coord = 0;
    y_coord = 0;
}

void loadSnapshot(Registers_t *regs)
{
    if (regs == NULL)
    {
        return;
    }

    memcpy(&snapshot, regs, sizeof(Registers_t));
    snapshotReady = true;
}

uint8_t isSpecialChar(char c) 
{
    return (c == '\n' || c == '\r' || c == '\t' || c == '\b');
}

uint64_t sys_write(uint8_t fd, const char *str, uint64_t count) 
{
    // Solo se soporta escritura en STDOUT
    if (fd != STDOUT) 
        return 0; 

    int width = getWidth();
    int height = getHeight();

    for (uint64_t i = 0; i < count; i++) 
    {
        if (isSpecialChar(str[i])) 
        {
            switch (str[i]) 
            {
            case '\n':
                y_coord += height + FONT_CHAR_GAP;
        
            case '\r':
                x_coord = 0;
                break;

            case '\t':
                x_coord += (TAB_SPACES * (width + FONT_CHAR_GAP));
                if (x_coord >= getScreenWidth()) 
                {
                    x_coord = x_coord % getScreenWidth();
                    y_coord += height + FONT_CHAR_GAP;
                }
                break;

            case '\b':

                if (x_coord > width + FONT_CHAR_GAP) 
                {
                    x_coord -= (width + FONT_CHAR_GAP);
                } 
                else 
                {
                    if (y_coord > 0) 
                    {
                        y_coord -= (height + FONT_CHAR_GAP);
                        x_coord = getScreenWidth() - TAB_SPACES*(width+ FONT_CHAR_GAP) + x_coord;
                    } 
                    else 
                    {
                        x_coord = 0;
                    }
                }

                // TODO: Esto en vez del un espacio tendria que ser un caracter con todo
                // el bit map en 1. Tambien sirve para el cursor
                drawRectangle(width, height, NEGRO, x_coord, y_coord);
                break;
            }
        } 

        // TODO: Aca se podria optimizar con getRemainingScreenWidth
        else if (isValidScreenPrint(x_coord, y_coord, width, height)) 
        {
            drawChar(str[i], BLANCO, x_coord, y_coord);
            x_coord += width + FONT_CHAR_GAP;
        } 
        else 
        {
            x_coord = 0;
            y_coord += height + FONT_CHAR_GAP;
        }
    }

    return count;
}

uint64_t sys_read(uint8_t fd, char *buffer, uint64_t count)
{
    if(fd == STDIN)
    {
        char c;

        for (uint64_t i = 0; i < count; i++)
        {
            if (!(c = kbd_get_char()))
                return i;

            buffer[i] = c;
        }
        
        return count;
    }

    return 0;
}

uint64_t get_pid()
{
    return sys_get_pid();
}

 
void syscallDispatcher(Registers_t *regs) 
{
    // El número de la syscall generalmente se pasa en RAX
    uint64_t syscall_id = regs->rax;

    // Los argumentos suelen pasarse en RDI, RSI, RDX, RCX, R8, R9 (según la convención de llamada de System V AMD64)
    uint64_t arg1 = regs->rdi;
    uint64_t arg2 = regs->rsi;
    uint64_t arg3 = regs->rdx;
    uint64_t arg4 = regs->rcx;
    uint64_t arg5 = regs->r8;
    uint64_t arg6 = regs->r9;

    /*
        Las que son 0x1... son syscalls de video
    */

    switch (syscall_id) 
    {
        case 0x1: 
            sys_write(arg1, (const char *)arg2, arg3);
            regs->rax = arg1;
            break;

        case 0x2:
            regs->rax = sys_read(arg1, (char *)arg2, arg3);
            break;

        case 0x04:
            if (!snapshotReady) {
                regs->rax = 1; // error: no hay snapshot
            } else {
                uint64_t *out = (uint64_t*)arg1;
                memcpy(out, &snapshot, sizeof(Registers_t));
                regs->rax = 0;
            }
            break;

        case 0x05:
            dateTime *dt = (dateTime *)arg1;
            getTime(dt);
            regs->rax = 0;
            break;

        case 0x06:
            zoomInFont();
            clearScreen();
            x_coord = 0;
            y_coord = 0;
            //drawString("ZOOM IN PERRO", VERDE, 200, 200);
            break;
        
        case 0x07:
            zoomOutFont();
            clearScreen();
            x_coord = 0;
            y_coord = 0;
            //drawString("ZOOM OUT PERRO", VIOLETA, 250, 250);
            break;

        case 0x10:
            clearScreen();
            x_coord = 0;
            y_coord = 0;
            regs->rax = 0;
            break;

        case 0x11:
            putPixel((uint32_t)arg1, arg2, arg3);
            regs->rax = 0;
            break;

        case 0x12:
            drawChar((char)arg1, (uint32_t)arg2, arg3, arg4);
            regs->rax = 0;
            break;

        case 0x13:
            drawString((const char *)arg1, (uint32_t)arg2, arg3, arg4);
            regs->rax = 0;
            break;

        case 0x14:
            drawRectangle(arg1, arg2, (uint32_t)arg3, arg4, regs->r8);
            regs->rax = 0;
            break;

        case 0x15:
            drawDecimal(arg1, (uint32_t)arg2, arg3, arg4);
            regs->rax = 0;
            break;

        case 0x16:
            drawHexa(arg1, (uint32_t)arg2, arg3, arg4);
            regs->rax = 0;
            break;

        case 0x17:
            drawBin(arg1, (uint32_t)arg2, arg3, arg4);
            regs->rax = 0;
            break;

        case 0x18:
            regs->rax = getScreenWidth();
            break;

        case 0x19:
            regs->rax = getScreenHeight();
            break;

        case 0x20:
            regs->rax = kbd_get_char();
            break;
        case 0x22:
            // getpid: devuelvo el pid del que me llamo. Lo unico que hace
            // es leer current_process->pid.
            regs->rax = get_pid();
            break;

        case 0x23:
            // exit: termina el proceso actual.
            // Marco KILLED y fuerzo un switch (int 0x20) porque sino seguiria
            // ejecutando codigo muerto hasta el proximo timer tick.
            // El scheduler libera la pagina cuando lo agarre.
            exit_process(current_process);
            force_schedule();
            break;

        case 0x24:
            // yield: el proceso suelta la CPU voluntariamente.
            // Igual que el timer tick: int 0x20 -> scheduler elige otro READY.
            // Cuando me vuelvan a elegir, vuelvo aca y popState me restaura los regs.
            force_schedule();
            break;

        case 0x25: {
            // kill(pid): mata a otro proceso (o a mi mismo).
            // Validacion: pid en rango y la entrada de la tabla tiene que existir.
            // Si el pid es invalido devuelvo -1 (errno style).
            uint64_t pid = arg1;
            if(pid == 0 || pid > MAX_PROCESSES || process_table[pid-1] == NULL) {
                regs->rax = (uint64_t)-1;
                break;
            }
            exit_process(process_table[pid-1]);  // misma logica que exit pero sobre otro PCB
            regs->rax = 0;
            // Caso especial: me mate a mi mismo. No puedo seguir corriendo, fuerzo switch.
            if(current_process != NULL && current_process->pid == pid) {
                force_schedule();
            }
            break;
        }

        case 0x26: {
            // block(pid): pone un proceso en BLOCKED. El scheduler lo va a saltar
            // hasta que alguien lo unblockee.
            uint64_t pid = arg1;
            if(pid == 0 || pid > MAX_PROCESSES || process_table[pid-1] == NULL) {
                regs->rax = (uint64_t)-1;
                break;
            }
            block_process(pid);
            regs->rax = 0;
            // Si me bloquee a mi mismo, fuerzo un switch sino seguiria corriendo
            // a pesar de estar marcado BLOCKED.
            if(current_process != NULL && current_process->pid == pid) {
                force_schedule();
            }
            break;
        }

        case 0x27: {
            // unblock(pid): saca al proceso de BLOCKED, lo deja READY para que
            // el scheduler lo pueda elegir de nuevo.
            uint64_t pid = arg1;
            if(pid == 0 || pid > MAX_PROCESSES || process_table[pid-1] == NULL) {
                regs->rax = (uint64_t)-1;
                break;
            }
            unblock_process(pid);
            regs->rax = 0;
            break;
        }

        case 0x28:
            // ps: el user me pasa un buffer y un max. Lleno el buffer con un
            // snapshot de la process_table (saltando NULL y KILLED) y devuelvo
            // cuantos escribi. La shell despues lo formatea como quiera.
            regs->rax = ps_snapshot((process_info_t *)arg1, (int)arg2);
            break;

        case 0x29:
            // mem_stats: lleno el struct del user con total/used/free en bytes.
            // El "total" es la memoria que el bitmap administra (post kernel y bitmap),
            // no la RAM fisica entera, asi el "used" arranca en ~0 y es legible.
            get_mem_stats((mem_info_t *)arg1);
            regs->rax = 0;
            break;

        // TODO: este no quedo organizado como los demas
        case 0x21:
            drawCircle(arg1, (uint32_t)arg2, arg3, arg4);
            regs->rax = 0;
            break;

        case 0x40:
            sleep(arg1);
            regs->rax = 0;
            break;

        default:
            // Syscall desconocida o no implementada
            // Se imprime un error o se establece un código de error en rax
            regs->rax = (uint64_t)-1; // Ejemplo de error (ENOSYS)
            break;
    }
}

