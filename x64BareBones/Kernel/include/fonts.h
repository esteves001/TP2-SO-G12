#ifndef _FONTS_H_
#define _FONTS_H_

#include <stdint.h>
#include <stddef.h>

#define FONTS 3

extern int currentFont;

// tamaño base SMALL (8×16)
#define FONT_CHAR_WIDTH_BYTES      8
#define FONT_CHAR_HEIGHT_BYTES     16

// escalados
#define FONT_CHAR_WIDTH_MEDIUM_BYTES   (FONT_CHAR_WIDTH_BYTES  * 2)
#define FONT_CHAR_HEIGHT_MEDIUM_BYTES  (FONT_CHAR_HEIGHT_BYTES * 2)
#define FONT_CHAR_WIDTH_LARGE_BYTES    (FONT_CHAR_WIDTH_BYTES  * 3)
#define FONT_CHAR_HEIGHT_LARGE_BYTES   (FONT_CHAR_HEIGHT_BYTES * 3)

#define FONT_CHAR_GAP        2
#define QNTY_PRINTABLE_CHARS 95
#define FIRST_PRINTABLE_CHAR 32
#define LAST_PRINTABLE_CHAR  126

// siempre máscara de 8 bits
#define FONT_MASK_SMALL 0x80

typedef const uint8_t font_char_t[FONT_CHAR_HEIGHT_BYTES];

typedef struct {
    const uint8_t *bitmap;  // &font_small[idx]
    int width;              // 8,16 o 24
    int height;             // 16,32 o 48
    uint32_t bitMask;       // siempre FONT_MASK_SMALL
} FontChar;

FontChar getCharBitMap(char c);
int getWidth(void);
int getHeight(void);
void zoomInFont(void);
void zoomOutFont(void);
extern int currentFont;

int getCurrentFontWidth(void);
int getCurrentFontHeight(void);

#endif