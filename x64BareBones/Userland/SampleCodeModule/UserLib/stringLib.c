#include <stringLib.h>

int strlen(const char *str) {
    int len = 0;
    while (*str++) len++;
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 != '\0' && *s2 != '\0') {
        if (*s1 != *s2)
            return *s1 - *s2;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}
