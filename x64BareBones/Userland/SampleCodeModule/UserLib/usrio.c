#include <usrio.h>

#define STDIN  0
#define STDOUT 1

extern char getchar(void)
{
    char c;
    while (!sys_read(STDIN, &c, 1));
    return c;
}

int putchar(char c)
{
    sys_write(STDOUT, &c, 1);
    return c;
}

// Devuelve la len + 1 por la \n
int puts(const char *s)
{
    uint64_t len;
    for (len = 0; s[len]; len++)
        putchar(s[len]);
    putchar('\n');
    return len + 1;
}

/*
    Para estos me gustaria usar las funciones drawDec, drawHexa 
    y drawBin.
    Revisar todo devuelta si se puede cambiar
*/

static void printUnsigned(unsigned int u) {
    if (u >= 10) printUnsigned(u / 10);
    putchar('0' + (u % 10));
}

static void printInt(int d) {
    if (d < 0)
    {
        putchar('-'); 
        d = -d;
    }
        
    printUnsigned((unsigned)d);
}

static void printHex(unsigned int x) {
    char buf[8];
    int i = 0;
    if (x == 0) { putchar('0'); return; }
    while (x) {
        int d = x & 0xF;
        buf[i++] = d < 10 ? '0' + d : 'a' + (d - 10);
        x >>= 4;
    }
    while (i--) putchar(buf[i]);
}

int printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            putchar(*p);
            continue;
        }
        switch (*++p) {
        case 'd': printInt(va_arg(ap, int)); break;
        case 'u': printUnsigned(va_arg(ap, unsigned)); break;
        case 'x': printHex(va_arg(ap, unsigned)); break;
        case 'c': putchar((char)va_arg(ap, int)); break;
        case 's': {
            char *s = va_arg(ap, char*);
            while (*s) putchar(*s++);
            break;
        }
        case '%': putchar('%'); break;
        }
    }
    va_end(ap);
    return 0;
}

// TODO: Revisar esto devuelta para ver si se puede mejorar
int scanf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int cnt = 0;
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') continue;
        char c, spec = *++p;
        if (spec == 'd') {
            int *ip = va_arg(ap, int*);
            int neg = 0, v = 0;
            while ((c = getchar()) <= ' ');
            if (c == '-') { neg = 1; c = getchar(); }
            for (; c >= '0' && c <= '9'; c = getchar())
                v = v * 10 + (c - '0');
            *ip = neg ? -v : v;
            cnt++;
        } else if (spec == 's') {
            char *s = va_arg(ap, char*);
            while ((c = getchar()) <= ' ');
            for (; c > ' '; c = getchar())
                *s++ = c;
            *s = 0;
            cnt++;
        }
    }
    va_end(ap);
    return cnt;
}

void to_lower(char * str){
    for (int i=0; str[i] != 0; i++){
        if ('A' <= str[i] && str[i] <= 'Z'){
            str[i] += ('a'-'A');
        }
    }
}


uint64_t get_regist(uint64_t *registers){
    return sys_getRegisters(registers);
}
