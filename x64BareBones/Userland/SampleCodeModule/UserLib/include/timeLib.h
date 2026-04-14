#ifndef USERLIB_TIME_H
#define USERLIB_TIME_H

#include <stdint.h>

typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
}dateTime;

void getDateTime(dateTime *dt);
extern void sleepMilli(uint64_t milliseconds);

#endif