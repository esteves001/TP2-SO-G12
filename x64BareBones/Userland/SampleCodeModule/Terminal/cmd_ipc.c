#include <cmd_ipc.h>
#include <usrio.h>
#include <syscallLib.h>

#define STDIN  0
#define STDOUT 1

static int is_vowel(char c) {
    return c=='a'||c=='e'||c=='i'||c=='o'||c=='u'||
           c=='A'||c=='E'||c=='I'||c=='O'||c=='U';
}

void cmd_cat(int argc, char **argv) {
    char c;
    while (sys_read(STDIN, &c, 1) > 0)
        sys_write(STDOUT, &c, 1);
    sys_exit();
}

void cmd_wc(int argc, char **argv) {
    char c;
    int lines = 0;
    while (sys_read(STDIN, &c, 1) > 0)
        if (c == '\n') lines++;
    printf("%d\n", lines);
    sys_exit();
}

void cmd_filter(int argc, char **argv) {
    char c;
    while (sys_read(STDIN, &c, 1) > 0)
        if (is_vowel(c)) sys_write(STDOUT, &c, 1);
    sys_exit();
}
