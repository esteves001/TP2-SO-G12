#include <shell.h>

#define BUFFER 500
#define COMMAND_SIZE 14
#define SPECIAL_KEY_MAX_VALUE 5

char* commands_str[] = {
    "help", "exception 1", "exception 2",
    "zoom in", "zoom out",
     "clear", "date", "registers", "busywait", "busywaitkernel", "mem", "ps", "test_prio", "test_sync", "exit"
    };


typedef void (*ShellCommand)();
static ShellCommand commands[] = {
    help, exception_1, exception_2,
    zoom_in, zoom_out,
    clear_screen, printDateTime, getRegisters,
    busy_wait, busy_wait_kernel, mem_cmd, ps_cmd, test_prio_cmd, test_sync_cmd, exitShell};

char *registers[] = {
    " RAX: ", " RBX: ", " RCX: ",
     " RDX: ", " RSI: ", " RDI: ",
      " RBP: ", " RSP: ", " R8: ",
       " R9: ", " R10: ", " R11: ",
        " R12: ", " R13: ", " R14: ",
         " R15: ", " RIP: "};

void show_prompt() {
    printf("user@itba:> ");
}

static uint8_t active = 1;

void startShell(){
    char input_buffer[BUFFER];
    while(active) {
        show_prompt();
        readInput(input_buffer);
        to_lower(input_buffer);
        command_id command = processInput(input_buffer);
        if(command != -1) {
             commands[command]();
         }else {
             notACommand(input_buffer);
         }
    }
}

// Falta implementar la syscall read
void readInput(char *buffer){
    char * c = buffer;
    int limit_count = 0;

    do {
        *c = getchar();
        if(*c <= SPECIAL_KEY_MAX_VALUE) {
            c--;
        } else if(*c == '\b') {
            if(c > buffer) {
                putchar(*c);
                c--;
            }
            c--;
        }else {
            putchar(*c);
            limit_count++;
            if(limit_count > BUFFER)
                break;
        }
    } while((*c++) != '\n');
    *(c-1) = '\0';
}

command_id processInput(char* input){
    int index = -1;
    for(int i = 0; i < COMMAND_SIZE && (index == -1); i++) {
        if(strcmp(input, commands_str[i]) == 0){
            index = i;
        }
    }
    return index;
}

// Imprime todos los comandos disponibles
void help(){
    printf("Commands:\n");
    for(int i = 0; i < COMMAND_SIZE; i++) {
        printf("\t- %s\n", commands_str[i]);
    }
}

// Faltan las syscals del dateTime
void printDateTime(){
    dateTime dt;
    getDateTime(&dt);
    printf(" %d/%d/%d %d:%d:%d\n", dt.day, dt.month, dt.year, dt.hour, dt.min, dt.sec);
}

// Nombres de los registros para el comando 'registers'
// El orden DEBE COINCIDIR con la estructura Registers_t en syscalls.h
static const char* register_names[] = {
    "R15", "R14", "R13", "R12", "R11", "R10", "R9 ", "R8 ",
    "RSI", "RDI", "RBP", "RDX", "RCX", "RBX", "RAX",
    "RIP", "CS ", "FLG", "RSP", "SS "
};

void getRegisters() {
    uint64_t registers[20];
    if (get_regist(registers) != 0) {
        printf("No snapshot available. Press Ctrl+R first.\n");
    } else {
        printf("Register snapshot:\n");
        for (int i = 0; i < 20; i++) {
            printf("  %s: %x\n", register_names[i], registers[i]);
        }
    }
}

void notACommand(char* input){
    printf("Command \"%s\" not found. Type 'help' for a list of commands.\n", input);
}

void clear_screen() {
    clearScreen();
}

void zoom_in() {
    zoomIn();
}

void zoom_out() {
    zoomOut();
}

void exitShell(){
    printf("\n");
    printf("Exiting...\n");

    printf("\n[Exit succesful]\n");
    active = 0;
}

void exception_1(){
    int a = 1/0; 
}
void exception_2(){
    opCodeException();
}

/*
    Esto es exclusivamente para provar que cuando
    hacemos el ctrl + r dentro de userland el rip
    va a estar en la parte de userland y no en kernel.
    Porque con los juegos o la terminal esta siempre en
    el kernel esperando una tecla, y con sleep tambien
    lo mismo.
*/
void busy_wait() {
    printf("Running userland busy-wait. Press Ctrl+R now.\n");
    
    // El volatile es para que el compilador no lo optimice
    volatile uint64_t counter = 0;
    for (uint64_t i = 0; i < 1000000000; i++) {
        counter += i;
    }

    printf("Userland busy-wait finished.\n");
}

void busy_wait_kernel() {
    printf("Running kernel busy-wait. Press Ctrl+R now.\n");

    sleepMilli(5000);

    printf("Kernel busy-wait finished.\n");
}

// pido al kernel cuanta memoria hay total/usada/libre y la imprimo en KB.
// el "total" no es la RAM fisica entera, es lo que el bitmap puede dar
// (ya restado el kernel y el bitmap mismo). granularidad es de pagina (4KB).
void mem_cmd() {
    mem_info_t info;
    sys_mem_stats(&info);
    printf("Memory:\n");
    printf("  total: %d KB\n", info.total_bytes / 1024);
    printf("  used:  %d KB\n", info.used_bytes / 1024);
    printf("  free:  %d KB\n", info.free_bytes / 1024);
}

// chiquito helper para no imprimir 0/1/2 sino el nombre del estado.
static const char * state_str(int s) {
    if (s == 0) return "READY";
    if (s == 1) return "RUNNING";
    if (s == 2) return "BLOCKED";
    return "??";
}

// le paso al kernel un buffer mio (en el stack) y el max que entra.
// me devuelve cuantos procesos copio adentro. itero e imprimo pid/name/state.
// priority y foreground por ahora son placeholders, no los muestro.
#define MAX_PS 32
void ps_cmd() {
    process_info_t buf[MAX_PS];
    int n = sys_ps(buf, MAX_PS);
    printf("PID\tNAME\tSTATE\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%s\t%s\n", buf[i].pid, buf[i].name, state_str(buf[i].state));
    }
}

// lanza el test_prio: 3 workers en y=300/320/340 con prios 1/3/5. el azul (C)
// deberia terminar primero. no espera a que terminen, vuelve al prompt.
void test_prio_cmd() {
    test_prio_main(0, NULL);
    printf("test_prio lanzado: 3 workers (A rojo prio1, B verde prio3, C azul prio5).\n");
    printf("C deberia llegar al '*' primero.\n");
}

void test_sync_cmd() {
    return;
}