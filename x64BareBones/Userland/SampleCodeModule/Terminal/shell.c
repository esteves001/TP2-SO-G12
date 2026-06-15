#include <shell.h>
#include <cmd_ipc.h>
#include <cmd_proc.h>

#define BUFFER   512
#define MAX_ARGS 16
#define CTRL_C   3
#define CTRL_D   4

// --- Tipo de Command function ---
typedef void (*CmdFn)(int argc, char **argv);

typedef struct {
    const char *name;
    CmdFn       fn;
    int         can_bg; // 1 si puede ejecutarse como proceso (fn llama sys_exit al terminar)
} cmd_t;


static void cmd_help(int argc, char **argv);
static void cmd_test(int argc, char **argv);
static void cmd_clear(int argc, char **argv);
static void cmd_date(int argc, char **argv);
static void cmd_registers(int argc, char **argv);
static void cmd_busywait(int argc, char **argv);
static void cmd_busywaitkernel(int argc, char **argv);
static void cmd_mem(int argc, char **argv);
static void cmd_ps(int argc, char **argv);
static void cmd_exception1(int argc, char **argv);
static void cmd_exception2(int argc, char **argv);
static void cmd_zoom_in(int argc, char **argv);
static void cmd_zoom_out(int argc, char **argv);
static void cmd_test_mm(int argc, char **argv);
static void cmd_test_proc(int argc, char **argv);
static void cmd_test_prio(int argc, char **argv);
static void cmd_test_sync(int argc, char **argv);
static void cmd_test_pipe(int argc, char **argv);

// can_bg=0 → solo fg directo; can_bg=1 → puede ser proceso (llama sys_exit al terminar)
static cmd_t cmd_table[] = {
    {"help",           cmd_help,           0},
    {"test",           cmd_test,           0},
    {"clear",          cmd_clear,          0},
    {"date",           cmd_date,           0},
    {"registers",      cmd_registers,      0},
    {"busywait",       cmd_busywait,       0},
    {"busywaitkernel", cmd_busywaitkernel, 0},
    {"mem",            cmd_mem,            0},
    {"ps",             cmd_ps,             0},
    {"exception1",     cmd_exception1,     0},
    {"exception2",     cmd_exception2,     0},
    {"zoom_in",        cmd_zoom_in,        0},
    {"zoom_out",       cmd_zoom_out,       0},
    {"test_mm",        cmd_test_mm,        1},
    {"test_proc",      cmd_test_proc,      1},
    {"test_prio",      cmd_test_prio,      1},
    {"test_sync",      cmd_test_sync,      1},
    {"test_pipe",      cmd_test_pipe,      1},
    {"cat",            cmd_cat,            1},
    {"wc",             cmd_wc,             1},
    {"filter",         cmd_filter,         1},
    {"mvar",           cmd_mvar,           1},
    {"loop",           cmd_loop,           1},
    {"kill",           cmd_kill,           1},
    {"nice",           cmd_nice_cmd,       1},
    {"block",          cmd_block,          1},
    {(void*)0, (void*)0, 0}
};

static uint8_t active = 1;

// --- Helpers ---

static cmd_t *find_cmd(const char *name) {
    for (int i = 0; cmd_table[i].name != (void*)0; i++)
        if (strcmp(cmd_table[i].name, name) == 0)
            return &cmd_table[i];
    return (void*)0;
}

static int tokenize(char *str, char **argv, int max) {
    int argc = 0;
    char *p = str;
    while (*p && argc < max) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p == ' ') *p++ = '\0';
    }
    return argc;
}

// Lanza fn como proceso fg: crea proceso, registra fg_pid, espera.
static void run_as_process_fg(CmdFn fn, int argc, char **argv, int pipe_in_id, int pipe_out_id) {
    int64_t pid = sys_create_process((void *)fn, argv[0], argc, argv, pipe_in_id, pipe_out_id);
    if (pid < 0) { printf("error: proceso no creado\n"); return; }
    sys_set_fg_pid(pid);
    sys_waitpid(pid);
    sys_set_fg_pid(0);
}

// Lanza fn como proceso bg.
static void run_as_process_bg(CmdFn fn, int argc, char **argv, int pipe_in_id, int pipe_out_id) {
    int64_t pid = sys_create_process((void *)fn, argv[0], argc, argv, pipe_in_id, pipe_out_id);
    if (pid < 0) { 
        printf("error: proceso no creado\n"); 
        return; 
    }
    sys_set_foreground(pid, 0);
    printf("[bg] pid %d\n", (int)pid);
}

static void execute_line(char *line) {
    int len = strlen(line);
    while (len > 0 && line[len-1] == ' ') len--;
    line[len] = '\0';
    if (len == 0) return;

    // 1. Detectar '&' al final
    int is_bg = 0;
    if (len > 0 && line[len-1] == '&') {
        is_bg = 1;
        line[--len] = '\0';
        while (len > 0 && line[len-1] == ' ') line[--len] = '\0';
    }

    // 2. Detectar '|'
    char *pipe_pos = (void*)0;
    for (int i = 0; i < len; i++) {
        if (line[i] == '|') { pipe_pos = &line[i]; break; }
    }

    if (pipe_pos != (void*)0) {
        *pipe_pos = '\0';
        char *left  = line;
        char *right = pipe_pos + 1;

        char *argv_l[MAX_ARGS]; int argc_l = tokenize(left,  argv_l, MAX_ARGS);
        char *argv_r[MAX_ARGS]; int argc_r = tokenize(right, argv_r, MAX_ARGS);

        if (argc_l == 0 || argc_r == 0) { printf("syntax error\n"); return; }
        to_lower(argv_l[0]);
        to_lower(argv_r[0]);

        cmd_t *cl = find_cmd(argv_l[0]);
        cmd_t *cr = find_cmd(argv_r[0]);
        if (!cl) { printf("%s: comando no encontrado\n", argv_l[0]); return; }
        if (!cr) { printf("%s: comando no encontrado\n", argv_r[0]); return; }
        if (!cl->can_bg) { printf("%s: no soporta pipes\n", argv_l[0]); return; }
        if (!cr->can_bg) { printf("%s: no soporta pipes\n", argv_r[0]); return; }

        int pipe_id = sys_alloc_pipe();
        if (pipe_id < 0) { printf("error: pipe\n"); return; }

        int64_t pid_l = sys_create_process((void *)cl->fn, argv_l[0], argc_l, argv_l,
                                           0, pipe_id);
        if (pid_l < 0) { printf("error: proceso izquierdo no creado\n"); sys_close_pipe(pipe_id); return; }

        int64_t pid_r = sys_create_process((void *)cr->fn, argv_r[0], argc_r, argv_r,
                                           pipe_id, 0);
        if (!is_bg) {
            if (pid_l > 0) { sys_set_fg_pid(pid_l); sys_waitpid(pid_l); }
            if (pid_r > 0) { sys_set_fg_pid(pid_r); sys_waitpid(pid_r); }
            sys_set_fg_pid(0);
            printf("\n");
        } else {
            if (pid_l > 0) sys_set_foreground(pid_l, 0);
            if (pid_r > 0) sys_set_foreground(pid_r, 0);
        }
        sys_close_pipe(pipe_id);
        return;
    }

    // 3. Comando simple
    char *argv[MAX_ARGS];
    int argc = tokenize(line, argv, MAX_ARGS);
    if (argc == 0) return;
    to_lower(argv[0]);

    if (strcmp(argv[0], "exit") == 0) { 
        exitShell(); 
        return;
    }

    cmd_t *cmd = find_cmd(argv[0]);
    if (!cmd) { 
        notACommand(argv[0]); 
        return; 
    }

    if (is_bg) {
        if (!cmd->can_bg) {
            printf("%s: no soporta background, ejecutando en fg\n", argv[0]);
            cmd->fn(argc, argv);
        } else {
            run_as_process_bg(cmd->fn, argc, argv, 0, 0);
        }
    } else {
        if (cmd->can_bg) {
            // Comandos proceso: crear proceso fg para que sys_exit sea correcto
            run_as_process_fg(cmd->fn, argc, argv, 0, 0);
        } else {
            // Comandos directos: llamar en contexto de la shell
            cmd->fn(argc, argv);
        }
    }
}

// --- Loop principal de la shell ---

void show_prompt(void) {
    printf("user@itba:> ");
}

void readInput(char *buffer) {
    char *c = buffer;
    int limit = 0;
    do {
        char ch = getchar();
        if (ch == CTRL_C) {
            printf("^C\n");
            buffer[0] = '\0';
            return;
        }
        if (ch == CTRL_D) {
            // el kernel ya imprime el '\n' del EOF (sys_read), asi que aca no bajo de linea
            exitShell();     // imprime "Goodbye." y pone active = 0, igual que el comando exit
            buffer[0] = '\0';
            return;
        }
        *c = ch;
        if (*c > 0 && *c <= 5 && *c != '\n' && *c != '\b' && *c != '\t') {
            c--;
        } else if (*c == '\b') {
            if (c > buffer) {
                putchar(*c);
                c -= 2;
            } else {
                c--;
            }
        } else {
            putchar(*c);
            if (++limit > BUFFER) break;
        }
    } while ((*c++) != '\n');
    *(c - 1) = '\0';
}

void startShell(void) {
    char buf[BUFFER];
    printf("Bienvenido a la shell.\n");
    printf("Escribi 'help' para ver los comandos y 'test' para ver los tests.\n\n");
    while (active) {
        show_prompt();
        readInput(buf);
        if (!active) break;
        execute_line(buf);
    }
    sys_exit(); // soy el init: si salgo del loop, termino limpio en vez de volver a main y descarrilar
}


static void print_padded(const char *name, int pad) {
    int len = 0;
    while (name[len]) { putchar(name[len]); len++; }
    for (int i = len; i < pad; i++) putchar(' ');
}

static int is_test_cmd(const char *n) {
    return n[0]=='t'&&n[1]=='e'&&n[2]=='s'&&n[3]=='t'&&n[4]=='_';
}

static void cmd_help(int argc, char **argv) {
    printf("Lista de comandos:\n");
    int col = 0;
    for (int i = 0; cmd_table[i].name != (void*)0; i++) {
        const char *n = cmd_table[i].name;
        if (is_test_cmd(n)) continue;          // los tests van en 'test'
        if (strcmp(n, "test") == 0) continue;  // no listar el propio 'test'
        putchar(' ');
        print_padded(n, 14);
        if (++col % 3 == 0) putchar('\n');
    }
    if (col % 3 != 0) putchar('\n');
    print_padded(" exit", 15);
    putchar('\n');
}

static void cmd_test(int argc, char **argv) {
    printf("Lista de tests:\n");
    printf("  test_mm\n");
    printf("  test_proc\n");
    printf("  test_prio\n");
    printf("  test_sync\n");
    printf("  test_pipe\n");
}

static void cmd_clear(int argc, char **argv) {
    clearScreen(); // syscall 0x10 ya resetea x_coord/y_coord en el kernel
}

static void cmd_zoom_in(int argc, char **argv)  { zoomIn();  }
static void cmd_zoom_out(int argc, char **argv) { zoomOut(); }

static void cmd_date(int argc, char **argv) {
    dateTime dt;
    getDateTime(&dt);
    printf("%d/%d/%d %d:%d:%d\n", dt.day, dt.month, dt.year, dt.hour, dt.min, dt.sec);
}

static const char *register_names[] = {
    "R15","R14","R13","R12","R11","R10","R9 ","R8 ",
    "RSI","RDI","RBP","RDX","RCX","RBX","RAX",
    "RIP","CS ","FLG","RSP","SS "
};

static void cmd_registers(int argc, char **argv) {
    uint64_t regs[20];
    if (get_regist(regs) != 0) {
        printf("Sin snapshot. Presiona Ctrl+R primero.\n");
        return;
    }
    for (int i = 0; i < 20; i++)
        printf("  %s: %x\n", register_names[i], regs[i]);
}

static void cmd_busywait(int argc, char **argv) {
    printf("Userland busy-wait. Presiona Ctrl+R ahora.\n");
    volatile uint64_t x = 0;
    for (uint64_t i = 0; i < 1000000000ULL; i++) x += i;
    printf("Listo.\n");
}

static void cmd_busywaitkernel(int argc, char **argv) {
    printf("Kernel busy-wait 5s...\n");
    sleepMilli(5000);
    printf("Listo.\n");
}

static void cmd_mem(int argc, char **argv) {
    mem_info_t info;
    sys_mem_stats(&info);
    printf("total: %d KB\nused:  %d KB\nfree:  %d KB\n",
           (int)(info.total_bytes / 1024),
           (int)(info.used_bytes  / 1024),
           (int)(info.free_bytes  / 1024));
}

static const char *state_str(int s) {
    if (s == 0) return "READY  ";
    if (s == 1) return "RUNNING";
    if (s == 2) return "BLOCKED";
    return "??     ";
}

#define MAX_PS 16
static void cmd_ps(int argc, char **argv) {
    process_info_t buf[MAX_PS];
    int n = sys_ps(buf, MAX_PS);
    printf("PID\tNAME\tSTATE\tPRIO\tFG\tSP\tBP\n");
    for (int i = 0; i < n; i++) {
        printf("%d", (int)buf[i].pid);        // id del proceso
        putchar('\t');
        for (int j = 0; j < MAX_PROCESS_NAME && buf[i].name[j]; j++)
            putchar(buf[i].name[j]);          // nombre
        putchar('\t');
        printf("%s", state_str(buf[i].state)); // estado: ready/running/blocked
        putchar('\t');
        printf("%d", (int)buf[i].priority);   // prioridad (1 a 5)
        putchar('\t');
        printf("%d", buf[i].foreground);      // 1 si corre en foreground, 0 si background
        putchar('\t');
        printf("0x%lx", buf[i].rsp);          // stack pointer
        putchar('\t');
        printf("0x%lx", buf[i].rbp);          // base pointer (RBP)
        putchar('\n');
    }
}

static void cmd_exception1(int argc, char **argv) { volatile int cero = 0; int a = 1/cero; (void)a; }
static void cmd_exception2(int argc, char **argv) { opCodeException(); }

// Comandos proceso (can_bg=1): DEBEN llamar sys_exit() al terminar.
static void cmd_test_mm(int argc, char **argv) {
    test_mm(argc - 1, argv + 1);
    sys_exit();
}
static void cmd_test_proc(int argc, char **argv) {
    test_proc(argc - 1, argv + 1);
    sys_exit();
}
static void cmd_test_prio(int argc, char **argv) {
    test_prio(argc - 1, argv + 1);
    sys_exit();
}
static void cmd_test_sync(int argc, char **argv) {
    test_sync(argc - 1, argv + 1);
    sys_exit();
}
static void cmd_test_pipe(int argc, char **argv) {
    test_pipe_main(argc, argv);
    sys_exit();
}


void notACommand(char *input) {
    printf("'%s': comando no encontrado. Escribi 'help'.\n", input);
}

void exitShell(void) {
    printf("Goodbye.\n");
    active = 0;
}