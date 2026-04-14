#ifndef _TIME_H_
#define _TIME_H_

#include <stdint.h>
#include <interrupts.h>

typedef struct{
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
}dateTime;

void timer_handler();
int ticks_elapsed();
int seconds_elapsed();
uint64_t ms_elapsed();
void sleep(uint64_t milliseconds);

void getTime(dateTime *dt);

uint16_t getSec();
uint16_t getMin();
uint16_t  getHour();
uint16_t getDay();
uint16_t  getMonth();
uint16_t getYear();

int bcd_to_dec(int bcd);

// Tiempo RTC
extern unsigned int getSysSeconds(void);
extern unsigned int getSysMinutes(void);
extern unsigned int getSysHours(void);
extern unsigned int getSysDayOfWeek(void);
extern unsigned int getSysDayOfMonth(void);
extern unsigned int getSysMonth(void);
extern unsigned int getSysYear(void);

#endif
