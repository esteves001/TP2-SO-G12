#include <shell.h>

#define BUFFER 500
#define COMMAND_SIZE 12
#define SPECIAL_KEY_MAX_VALUE 5

char* commands_str[] = {
    "help", "exception 1", "exception 2", 
    "pongisgolf", "zoom in", "zoom out",
     "clear", "date", "registers", "busywait", "busywaitkernel", "exit"
    };


typedef void (*ShellCommand)();
static ShellCommand commands[] = {
    help, exception_1, exception_2, 
    startPongis, zoom_in, zoom_out, 
    clear_screen, printDateTime, getRegisters,
    busy_wait, busy_wait_kernel, exitShell};

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

void startPongis(){
    printf("Starting Pongis Golf...\n");
    startPongisGolf();
    clear_screen();
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