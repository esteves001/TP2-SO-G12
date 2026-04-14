#ifndef _LIB_H
#define _LIB_H

#include <stdint.h>
#include <syscallLib.h>

void clearScreen(void);
void putPixel(uint32_t hexColor, uint64_t x, uint64_t y);
void drawChar(char c, uint32_t hexColor, uint64_t x, uint64_t y);
void drawString(const char* str, uint32_t hexColor, uint64_t x, uint64_t y);
void drawRectangle(uint64_t width, uint64_t heigth, uint32_t hexColor, uint64_t x, uint64_t y);
void drawCircle(uint64_t radius, uint32_t hexColor, uint64_t x, uint64_t y); // Esta función no estaba implementada en el código original
void drawDecimal(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y);
void drawHexa(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y); // Ya estaba declarada
void drawBin(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y);

uint16_t getScreenWidth();
uint16_t getScreenHeight();

uint16_t getCharWidth();
uint16_t getCharHeight();
uint16_t getCharGap();

void paintScreen(uint32_t hexColor);

void zoomIn();
void zoomOut();

#endif