#include <timeLib.h>

void getDateTime(dateTime *dt) {
    sys_getDateTime(dt); 
}

extern void sleepMilli(uint64_t milliseconds){
    sys_sleepMilli(milliseconds);
}