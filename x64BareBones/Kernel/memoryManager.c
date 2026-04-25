#include <memoryManager.h>

// Dirección física donde Pure64 deja la cantidad de RAM en MiB (32-bit)
#define PURE64_RAM_INFO 0x5020

// Dirección segura para comenzar a administrar la memoria
// (0x100000 Kernel, 0x400000 CodeModule, 0x500000 DataModule)
#define MANAGED_MEMORY_START 0x600000 

// Ubicamos el Bitmap 
static uint8_t* bitmap = (uint8_t*)MANAGED_MEMORY_START;


static uint64_t total_pages = 0;
static uint64_t free_pages = 0;
static uint64_t first_free_page = 0;

// Idea
// 1) Encontrar cuantos marcos de paginas entran en la RAM (total_pages)
// 2) Encontrar cuantos marcos de paginas va a tener el Bitmap (bitmap_pages)
// 3) Ponemos como ocupados todos los marcos de paginas
// 4) Ponemos como libres los marcos de pagina que realmente estan libres
// Asi dejamos la memoria del Kernel, etc. protegida e inaccesible
void initializeMemoryManager(void) {
    // 1. Leer cantidad de RAM desde la Information Table de Pure64
    uint32_t ram_mb = *((uint32_t*)PURE64_RAM_INFO);
    uint64_t total_ram_bytes = (uint64_t)ram_mb * 1024 * 1024; // Megabytes a Bytes 1024 * 1024
    
    total_pages = total_ram_bytes / PAGE_SIZE; 
    
    // 2. Calcular cuántas páginas ocupa el propio bitmap
    uint64_t bitmap_size_bytes = total_pages / 8;
    if (total_pages % 8 != 0) {
        bitmap_size_bytes++; // suma un byte extra si sobra algún resto (para no dejar páginas sin administrar)
    }
    // Calculamos cuántas páginas enteras se necesitan para cubir todo el mapa
    uint64_t bitmap_pages = (bitmap_size_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    // 3. Inicialmente marcamos toda la memoria como ocupada (bits en 1)
    for (uint64_t i = 0; i < bitmap_size_bytes; i++) {
        bitmap[i] = 0xFF;
    }

    // 4. Calculamos cual es la primera pagina libre (despues de Kernel, Modulos y el propio Bitmap)
    first_free_page = (MANAGED_MEMORY_START / PAGE_SIZE) + bitmap_pages;
    // A partir de este índice, todo el resto de la RAM hasta el final queda como memoria libre


    // 5. Liberar las páginas que realmente podemos usar
    for (uint64_t i = first_free_page; i < total_pages; i++) {
        uint64_t byte_index = i / 8;
        uint8_t bit_index = i % 8;
        bitmap[byte_index] &= ~(1 << bit_index); // Instrucción estándar en C para "apagar un bit"
    }
    
    free_pages = total_pages - first_free_page;
}

// Idea
// 1) Encontramos una posicion libre (0) en el bitmap
// 2) Pasamos dicha posicion a (1)
// 3) Devolvemos el puntero correspondiente al espacio en memoria que acabamos de reservar 
void* allocatePage(void) {
    // Iteramos por los marcos de pagina
    for (uint64_t i = first_free_page; i < total_pages; i++) {
        uint64_t byte_index = i / 8;
        uint8_t bit_index = i % 8;

        if ((bitmap[byte_index] & (1 << bit_index)) == 0) { // Si el bit es 0 (Libre)
            bitmap[byte_index] |= (1 << bit_index);         // Lo marcamos como 1 (Ocupado)
            free_pages--;
            return (void*)(i * PAGE_SIZE); // Devolvemos el puntero (dirección física exacta)
        }
    }
    return NULL; // Memoria llena
}

// Idea
// 1) Encontrar el marco de pagina correspondiente al puntero
// 2) Marcarlo como 0 (libre)
void freePage(void* address) {
    uint64_t page_index = (uint64_t)address / PAGE_SIZE; // Obtenemos el marco de pagina 
    bitmap[page_index / 8] &= ~(1 << (page_index % 8));   // Lo volvemos a poner en 0
    free_pages++;
}
// En la memoria fisica queda como "basura", cuando otro porceso solicite
// memoria, simplemente va a encontrar dicho marco de pagina libre y 
// va a pisar dicha "basura"