#include <lib.h>
#include <fonts.h>
#include <stdint.h>
#include <stddef.h>
#include <videoDriver.h>

struct vbe_mode_info_structure {
	uint16_t attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	uint8_t window_a;			// deprecated
	uint8_t window_b;			// deprecated
	uint16_t granularity;		// deprecated; used while calculating bank numbers
	uint16_t window_size;
	uint16_t segment_a;
	uint16_t segment_b;
	uint32_t win_func_ptr;		// deprecated; used to switch banks from protected mode without returning to real mode
	uint16_t pitch;			// number of bytes per horizontal line
	uint16_t width;			// width in pixels
	uint16_t height;		// height in pixels
	uint8_t w_char;			// unused...
	uint8_t y_char;			// ...
	uint8_t planes;
	uint8_t bpp;			// bits per pixel in this mode
	uint8_t banks;			// deprecated; total number of banks in this mode
	uint8_t memory_model;
	uint8_t bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	uint8_t image_pages;
	uint8_t reserved0;
 
	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;
	uint8_t direct_color_attributes;
 
	uint32_t framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
	uint8_t reserved1[206];
} __attribute__ ((packed));

typedef struct vbe_mode_info_structure * VBEInfoPtr;

VBEInfoPtr VBE_mode_info = (VBEInfoPtr) 0x0000000000005C00;

void putPixel(uint32_t hexColor, uint64_t x, uint64_t y) {
    uint8_t * framebuffer = (uint8_t *) VBE_mode_info->framebuffer;
    uint64_t offset = (x * ((VBE_mode_info->bpp)/8)) + (y * VBE_mode_info->pitch);
    framebuffer[offset]     =  (hexColor) & 0xFF;
    framebuffer[offset+1]   =  (hexColor >> 8) & 0xFF; 
    framebuffer[offset+2]   =  (hexColor >> 16) & 0xFF;
}

/*
	Nuestra parte de la implemntacion
*/

// TODO: Ninguna de estas funciones cheuqea el ancho y largo de la pantalla

uint16_t getScreenWidth()
{
    return VBE_mode_info->width;
}

uint16_t getScreenHeight()
{
    return VBE_mode_info->height;
}

uint8_t isValidScreenCoordinate(uint16_t x, uint16_t y)
{
    uint16_t width = getScreenWidth();
    uint16_t height = getScreenHeight();

    return x >= 0 && x <= width && y >= 0 && y <= height;
}

// Si queremos chequear si un string va a entrar en la pantalla hay que decirle el ancho y alto del string
// en pixeles

/*
    TODO: Arreglar esto
    Iba a agregar esta funcion para chequear si un string o un char entran en la pantalla,
    pero si lo agrego al char, cuando quiero imprimir un string lo tengo que chequear
    para cada caracter, asi que tendria que modificar mas las cosas para chequearlo
*/
uint8_t isValidScreenPrint(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    return isValidScreenCoordinate(x, y) && isValidScreenCoordinate(x + width - 1, y + height - 1);
}

uint16_t getRemainingScreenWidth(uint16_t x_coord)
{
    return getScreenWidth() - (x_coord);
}

static void putPixelIfValid(uint32_t color, int64_t x, int64_t y) {
    uint16_t w = getScreenWidth(), h = getScreenHeight();
    if (x >= 0 && x < w && y >= 0 && y < h)
        putPixel(color, x, y);
}

void drawChar(char c, uint32_t color, uint64_t x, uint64_t y) {
    FontChar f = getCharBitMap(c);
    if (!f.bitmap) return;
    int baseW = FONT_CHAR_WIDTH_BYTES;
    int baseH = FONT_CHAR_HEIGHT_BYTES;
    int scale = f.width / baseW;  // 1,2 o 3
    if (!isValidScreenPrint(x, y, f.width, f.height)) return;

    for (int row = 0; row < baseH; row++) {
        uint8_t bits = f.bitmap[row];
        for (int col = 0; col < baseW; col++) {
            if (bits & (f.bitMask >> col)) {
                for (int dy = 0; dy < scale; dy++)
                    for (int dx = 0; dx < scale; dx++)
                        putPixelIfValid(color,
                            x + col*scale + dx,
                            y + row*scale + dy);
            }
        }
    }
}

void drawString(const char* s, uint32_t hexColor, uint64_t x, uint64_t y) {
    int w = getWidth(), h = getHeight();
    for (const char *p = s; *p; p++) {
        if (!isValidScreenPrint(x, y, w, h)) {
            x = 0; y += h + FONT_CHAR_GAP;
            if (!isValidScreenPrint(x, y, w, h)) break;
        }
        drawChar(*p, hexColor, x, y);
        x += w + FONT_CHAR_GAP;
    }
}

void drawRectangle(uint64_t width, uint64_t heigth, uint32_t hexColor, uint64_t x, uint64_t y)
{   
    if (!isValidScreenPrint(x, y, width, heigth)) return;

	for (uint64_t i = x; i < x + width; i++)
	{
		for (uint64_t j = y; j < y + heigth; j++)
		{
			putPixel(hexColor, i, j);
		}
	}
}

static void plotCircleOctants(uint32_t hexColor, int64_t xc, int64_t yc, int64_t dx, int64_t dy)
{  
    putPixelIfValid(hexColor, xc + dx, yc + dy);
    putPixelIfValid(hexColor, xc - dx, yc + dy);
    putPixelIfValid(hexColor, xc + dx, yc - dy);
    putPixelIfValid(hexColor, xc - dx, yc - dy);
    putPixelIfValid(hexColor, xc + dy, yc + dx);
    putPixelIfValid(hexColor, xc - dy, yc + dx);
    putPixelIfValid(hexColor, xc + dy, yc - dx);
    putPixelIfValid(hexColor, xc - dy, yc - dx);
}

static void drawHLine(uint32_t hexColor, int64_t x1, int64_t x2, int64_t y) {
    if (x1 > x2) { int64_t t = x1; x1 = x2; x2 = t; }
    for (int64_t x = x1; x <= x2; x++) {
        putPixelIfValid(hexColor, x, y);
    }
}

// Dibuja los segmentos horizontales correspondientes a los 8 octantes,
// rellenando el círculo
static void fillCircleOctants(uint32_t hexColor, int64_t xc, int64_t yc, int64_t dx, int64_t dy)
{
    // Octantes “superiores” e “inferiores”
    drawHLine(hexColor, xc - dx, xc + dx, yc + dy);
    drawHLine(hexColor, xc - dx, xc + dx, yc - dy);
    // Octantes con roles invertidos
    drawHLine(hexColor, xc - dy, xc + dy, yc + dx);
    drawHLine(hexColor, xc - dy, xc + dy, yc - dx);
}

void drawCircle(uint64_t radius, uint32_t hexColor, uint64_t x_center, uint64_t y_center)
{
    int64_t xc = (int64_t)x_center;
    int64_t yc = (int64_t)y_center;
    int64_t r  = (int64_t)radius;

    if (r <= 0) {
        putPixelIfValid(hexColor, xc, yc);
        return;
    }

    int64_t x = 0;
    int64_t y = r;
    int64_t p = 1 - r;

    // Dibuja la línea central y sus simétricas
    fillCircleOctants(hexColor, xc, yc, x, y);

    while (x < y) {
        x++;
        if (p < 0) {
            p += 2*x + 1;
        } else {
            y--;
            p += 2*x + 1 - 2*y;
        }
        fillCircleOctants(hexColor, xc, yc, x, y);
    }
}

// Para estas funciones, la que chequa si entran en la pantalla es drawString
void drawDecimal(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y)
{
    char buffer[21];
    uint64ToString(value, buffer, 10);
    drawString(buffer, hexColor, x, y);
}

void drawHexa(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y)
{
    char buffer[19];
    buffer[0] = '0';
    buffer[1] = 'x';
    uint64ToString(value, buffer + 2, 16);
    drawString(buffer, hexColor, x, y);
}

void drawBin(uint64_t value, uint32_t hexColor, uint64_t x, uint64_t y)
{
    char buffer[67];
    buffer[0] = '0';
    buffer[1] = 'b';
    uint64ToString(value, buffer + 2, 2);
    drawString(buffer, hexColor, x, y);
}

/*
	TODO: Esto no se si esta bien asi o habria que borrar todo el buffer
*/

void clearScreen(void)
{
	drawRectangle(getScreenWidth(), getScreenHeight(), 0x00000000, 0, 0);
}