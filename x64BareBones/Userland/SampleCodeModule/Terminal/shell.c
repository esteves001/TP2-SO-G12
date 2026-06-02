#include <shell.h>

#define BUFFER        512
#define MAX_ARGS      16
#define SHELL_PIPE_ID 15
#define CTRL_C        3
#define CTRL_D        4

// --- Command function type ---
typedef void (*CmdFn)(int argc, char **argv);

typedef struct {
    const char *name;
    CmdFn       fn;
    int         can_bg; // 1 si puede ejecutarse como proceso (fn llama sys_exit al terminar)
} cmd_t;

// --- Forward declarations ---
static void cmd_help(int argc, char **argv);
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
static void cmd_test_prio(int argc, char **argv);
static void cmd_test_sync(int argc, char **argv);
static void cmd_test_pipe(int argc, char **argv);

// can_bg=0 → solo fg directo; can_bg=1 → puede ser proceso (llama sys_exit al terminar)
static cmd_t cmd_table[] = {
    {"help",           cmd_help,           0},
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
    {"test_prio",      cmd_test_prio,      1},
    {"test_sync",      cmd_test_sync,      1},
    {"test_pipe",      cmd_test_pipe,      1},
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
static void run_as_process_fg(CmdFn fn, int argc, char **argv,
                               int pipe_in_id, int pipe_out_id) {
    int64_t pid = sys_create_process((void *)fn, argv[0], argc, argv,
                                     pipe_in_id, pipe_out_id);
    if (pid < 0) { printf("error: proceso no creado\n"); return; }
    sys_set_fg_pid(pid);
    sys_waitpid(pid);
    sys_set_fg_pid(0);
}

// Lanza fn como proceso bg.
static void run_as_process_bg(CmdFn fn, int argc, char **argv,
                               int pipe_in_id, int pipe_out_id) {
    int64_t pid = sys_create_process((void *)fn, argv[0], argc, argv,
                                     pipe_in_id, pipe_out_id);
    if (pid < 0) { printf("error: proceso no creado\n"); return; }
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

        cmd_t *cl = find_cmd(argv_l[0]);
        cmd_t *cr = find_cmd(argv_r[0]);
        if (!cl) { printf("%s: comando no encontrado\n", argv_l[0]); return; }
        if (!cr) { printf("%s: comando no encontrado\n", argv_r[0]); return; }
        if (!cl->can_bg) { printf("%s: no soporta pipes\n", argv_l[0]); return; }
        if (!cr->can_bg) { printf("%s: no soporta pipes\n", argv_r[0]); return; }

        if (sys_create_pipe(SHELL_PIPE_ID) < 0) { printf("error: pipe\n"); return; }

        int64_t pid_l = sys_create_process((void *)cl->fn, argv_l[0], argc_l, argv_l,
                                           0, SHELL_PIPE_ID);
        int64_t pid_r = sys_create_process((void *)cr->fn, argv_r[0], argc_r, argv_r,
                                           SHELL_PIPE_ID, 0);
        if (!is_bg) {
            if (pid_l > 0) { sys_set_fg_pid(pid_l); sys_waitpid(pid_l); }
            if (pid_r > 0) { sys_set_fg_pid(pid_r); sys_waitpid(pid_r); }
            sys_set_fg_pid(0);
        } else {
            if (pid_l > 0) sys_set_foreground(pid_l, 0);
            if (pid_r > 0) sys_set_foreground(pid_r, 0);
        }
        sys_close_pipe(SHELL_PIPE_ID);
        return;
    }

    // 3. Comando simple
    char *argv[MAX_ARGS];
    int argc = tokenize(line, argv, MAX_ARGS);
    if (argc == 0) return;

    if (strcmp(argv[0], "exit") == 0) { exitShell(); return; }

    cmd_t *cmd = find_cmd(argv[0]);
    if (!cmd) { notACommand(argv[0]); return; }

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

// --- Shell main loop ---

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
            printf("\n");
            active = 0;
            buffer[0] = '\0';
            return;
        }
        *c = ch;
        if (*c > 0 && *c <= 5 && *c != '\n' && *c != '\b' && *c != '\t') {
            c--;
        } else if (*c == '\b') {
            if (c > buffer) {
                putchar(*c);
                c--;
            }
            c--;
        } else {
            putchar(*c);
            if (++limit > BUFFER) break;
        }
    } while ((*c++) != '\n');
    *(c - 1) = '\0';
}

void startShell(void) {
    char buf[BUFFER];
    while (active) {
        show_prompt();
        readInput(buf);
        if (!active) break;
        to_lower(buf);
        execute_line(buf);
    }
}

// --- Command implementations ---

static void cmd_help(int argc, char **argv) {
    printf("Comandos disponibles:\n");
    for (int i = 0; cmd_table[i].name != (void*)0; i++) {
        printf("  %s", cmd_table[i].name);
        if (cmd_table[i].can_bg) printf(" [bg/pipe]");
        printf("\n");
    }
    printf("  exit\n");
    printf("\nTests: test_prio [N]  test_sync <pairs> <iters> <use_sem>  test_pipe\n");
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
    printf("PID  NAME             STATE    PRIO FG\n");
    for (int i = 0; i < n; i++) {
        // Imprimir campo por campo para evitar problemas con va_args y %s
        printf("%d", (int)buf[i].pid);
        putchar('\t');
        // Imprimir nombre caracter a caracter (evita dependencia de null term)
        for (int j = 0; j < MAX_PROCESS_NAME && buf[i].name[j]; j++)
            putchar(buf[i].name[j]);
        putchar('\t');
        printf("%s", state_str(buf[i].state));
        putchar('\t');
        printf("%d", (int)buf[i].priority);
        putchar('\t');
        printf("%d", buf[i].foreground);
        putchar('\n');
    }
}

static void cmd_exception1(int argc, char **argv) { int a = 1/0; (void)a; }
static void cmd_exception2(int argc, char **argv) { opCodeException(); }

// Comandos proceso (can_bg=1): DEBEN llamar sys_exit() al terminar.
static void cmd_test_prio(int argc, char **argv) {
    test_prio_main(argc, argv);
    sys_exit();
}
static void cmd_test_sync(int argc, char **argv) {
    test_sync_main(argc, argv);
    sys_exit();
}
static void cmd_test_pipe(int argc, char **argv) {
    test_pipe_main(argc, argv);
    sys_exit();
}

// --- Funciones legacy ---
void notACommand(char *input) {
    printf("'%s': comando no encontrado. Escribi 'help'.\n", input);
}

void exitShell(void) {
    printf("Goodbye.\n");
    active = 0;
}
