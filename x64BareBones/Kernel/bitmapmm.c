#include <memoryManager.h>

// Direccion fisica donde Pure64 deja la cantidad de RAM en MiB (32-bit)
#define PURE64_RAM_INFO 0x5020

// Direccion segura para comenzar a administrar la memoria
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
void initialize_memory_manager(void) {
    // 1. Leer cantidad de RAM desde la Information Table de Pure64
    uint32_t ram_mb = *((uint32_t*)PURE64_RAM_INFO);
    uint64_t total_ram_bytes = (uint64_t)ram_mb * 1024 * 1024; // Megabytes a Bytes 1024 * 1024
    
    total_pages = total_ram_bytes / PAGE_SIZE; 
    
    // 2. Calcular cuantas paginas ocupa el propio bitmap
    uint64_t bitmap_size_bytes = total_pages / 8;
    if (total_pages % 8 != 0) {
        bitmap_size_bytes++; // suma un byte extra si sobra algun resto (para no dejar paginas sin administrar)
    }
    // Calculamos cuantas paginas enteras se necesitan para cubir todo el mapa
    uint64_t bitmap_pages = (bitmap_size_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    // 3. Inicialmente marcamos toda la memoria como ocupada (bits en 1)
    for (uint64_t i = 0; i < bitmap_size_bytes; i++) {
        bitmap[i] = 0xFF;
    }

    // 4. Calculamos cual es la primera pagina libre (despues de Kernel, Modulos y el propio Bitmap)
    first_free_page = (MANAGED_MEMORY_START / PAGE_SIZE) + bitmap_pages;
    // A partir de este indice, todo el resto de la RAM hasta el final queda como memoria libre


    // 5. Liberar las paginas que realmente podemos usar
    for (uint64_t i = first_free_page; i < total_pages; i++) {
        uint64_t byte_index = i / 8;
        uint8_t bit_index = i % 8;
        bitmap[byte_index] &= ~(1 << bit_index); // Instruccion estandar en C para "apagar un bit"
    }
    
    free_pages = total_pages - first_free_page;
}


// Busca `pages_needed` paginas contiguas libres a partir de first_free_page.
// Devuelve el indice de la primera pagina del bloque, o total_pages si no hay.
static uint64_t find_consecutive_pages(uint64_t pages_needed) {
    uint64_t consecutive = 0;
    uint64_t start_page  = 0;

    for (uint64_t i = first_free_page; i < total_pages; i++) {
        if ((bitmap[i / 8] & (1 << (i % 8))) == 0) {
            if (consecutive == 0) start_page = i;
            if (++consecutive == pages_needed) return start_page;
        } else {
            consecutive = 0;
        }
    }
    return total_pages; // no encontro
}

void* allocate_page(void) {
    uint64_t start = find_consecutive_pages(1);
    if (start == total_pages) return NULL;
    bitmap[start / 8] |= (1 << (start % 8));
    free_pages--;
    return (void*)(start * PAGE_SIZE);
}

void free_page(void* address) {
    uint64_t page_index = (uint64_t)address / PAGE_SIZE;
    bitmap[page_index / 8] &= ~(1 << (page_index % 8));
    free_pages++;
}

// Reserva un bloque de memoria de `size` bytes, alineado a paginas.
// Internamente busca paginas contiguas libres, las marca como ocupadas,
// y devuelve un puntero al bloque (con metadata de tamaño al inicio).
// Tambien maneja la asignacion de paginas individuales (size <= PAGE_SIZE).
void* allocate_block(uint64_t size) {
    if (size == 0) return NULL;

    uint64_t total_size   = size + sizeof(uint64_t);
    uint64_t pages_needed = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;

    uint64_t start = find_consecutive_pages(pages_needed);
    if (start == total_pages) return NULL;

    for (uint64_t i = start; i < start + pages_needed; i++)
        bitmap[i / 8] |= (1 << (i % 8));
    free_pages -= pages_needed;

    void* ptr = (void*)(start * PAGE_SIZE);
    *((uint64_t*)ptr) = pages_needed;
    return (void*)((uint8_t*)ptr + sizeof(uint64_t));
}


// Libera un bloque previamente asignado con allocate_block.
// Lee la metadata para obtener la cantidad de paginas a liberar,
// y las marca como disponibles en el bitmap.
// Funciona tanto para bloques individuales (paginas) como para bloques mayores.
void free_block(void* ptr) {
    if (ptr == NULL) return;

    uint8_t* actual      = (uint8_t*)ptr - sizeof(uint64_t);
    uint64_t pages_count = *((uint64_t*)actual);
    uint64_t start_page  = (uint64_t)actual / PAGE_SIZE;

    for (uint64_t i = start_page; i < start_page + pages_count; i++)
        bitmap[i / 8] &= ~(1 << (i % 8));
    free_pages += pages_count;
}

// Lleno el struct con el estado actual del bitmap. No agrego contador nuevo
// porque ya tengo total_pages y free_pages como static, y con eso me alcanza.
// El "total" que reporto es la memoria que el manager realmente puede dar
// (sin contar las paginas que se comen el kernel + bitmap), asi el "used"
// arranca cerca de 0 al boot y es legible para el comando mem.
// Si quisiera reportar la RAM fisica entera, seria total_pages * PAGE_SIZE.
void get_mem_stats(mem_info_t * out) {
    if(out == NULL) return;
    uint64_t manageable_pages = total_pages - first_free_page;
    out->total_bytes = manageable_pages * PAGE_SIZE;
    out->free_bytes  = free_pages * PAGE_SIZE;
    out->used_bytes  = (manageable_pages - free_pages) * PAGE_SIZE;
}