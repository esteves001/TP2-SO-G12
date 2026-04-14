#include <videoLib.h>

void clearScreen(void)
{
    sys_clearScreen();
}

void putPixel(uint32_t hexColor, uint64_t x, uint64_t y)
{
    sys_putPixel(hexColor, x, y);
}

void drawChar(char c, uint32_t hexColor, uint64_t x, uint64_t y) {
    sys_drawChar(c, hexColor, x, y);
}

void drawString(const char* str, uint32_t hexColor, uint64_t x, uint64_t y) {
    sys_drawString(str, hexColor, x, y);
}

void drawRectangle(uint64_t width, uint64_t height, uint32_t hexColor, uint64_t x, uint64_t y) {
    sys_drawRectangle(width, height, hexColor, x, y);
}

void drawDecimal(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y) {
    sys_drawDecimal(value, hexColor, x, y);
}

void drawHexa(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y) {
    sys_drawHexa(value, hexColor, x, y);
}

void drawBin(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y) {
    sys_drawBin(value, hexColor, x, y);
}

uint16_t getScreenWidth() {
    return sys_getScreenWidth();
}

uint16_t getScreenHeight() {
    return sys_getScreenHeight();
}

void paintScreen(uint32_t hexColor) {
    uint16_t width = getScreenWidth();
    uint16_t height = getScreenHeight();
    drawRectangle(width, height, hexColor, 0, 0);
}

void drawCircle(uint64_t radius, uint32_t hexColor, uint64_t x, uint64_t y) {
    sys_drawCircle(radius, hexColor, x, y);
}
void zoomIn() {
    sys_zoomIn();
}

void zoomOut() {
    sys_zoomOut();
}