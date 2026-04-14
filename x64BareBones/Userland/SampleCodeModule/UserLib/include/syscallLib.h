#ifndef _SYSCALL_LIB_H
#define _SYSCALL_LIB_H

#include <stdint.h>
#include <timeLib.h>

// Video
extern void sys_clearScreen(void);
extern void sys_putPixel(uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawChar(char c, uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawString(const char *str, uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawRectangle(uint64_t width, uint64_t height, uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawCircle(uint64_t radius, uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawDecimal(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawHexa(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y);
extern void sys_drawBin(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y);
extern uint16_t sys_getScreenWidth(void);
extern uint16_t sys_getScreenHeight(void);


// Teclado. 
// TODO: Esto habria que sacarlo creo porque ya tenemos sys_read
extern char sys_kbdGetChar(void);

// Shell
extern uint64_t sys_write(uint8_t fd, const char *str, uint64_t count);
extern uint64_t sys_read(uint8_t fd, char *buffer, uint64_t count);

extern void sys_zoomIn(void);
extern void sys_zoomOut(void);

extern void sys_getDateTime(dateTime *dt);
extern uint64_t sys_getRegisters(uint64_t *registers);
extern void sys_waitMilli(uint64_t milliseconds);

// Sonido
extern void sys_playSound(uint32_t frequency, uint32_t duration);

extern void opCodeException();

#endif