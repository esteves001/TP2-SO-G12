#include "memoryManager.h"

#define MAX_ORDER            10      
#define PURE64_RAM_INFO      0x5020
#define MANAGED_MEMORY_START 0x600000
#define KB_BYTES             1024

typedef struct block_node { //usamos double linked list porque es mas eficiente en el borrado 
    struct block_node *prev;
    struct block_node *next;
} block_node_t;

/* free_lists[0] = bloques de 4KB
   free_lists[1] = bloques de 8KB
   ...
   free_lists[10]= bloques de 4MB */
static block_node_t *free_lists[MAX_ORDER + 1];

static uint64_t total_manageable_bytes = 0;
static uint64_t free_bytes             = 0;
static uint8_t *buddy_base             = NULL;


static void list_push(block_node_t **head, block_node_t *node) { //inserta al principio de la lista
    node->next = *head;
    node->prev = NULL;

    if (*head) (*head)->prev = node;
    
    *head = node;
}

static void list_remove(block_node_t **head, block_node_t *node) { //remueve un nodo del medio de la lista
    if (node->prev) {
        node->prev->next = node->next; 
    } else *head = node->next;

    if (node->next) node->next->prev = node->prev;

    node->prev = node->next = NULL;
}

static int list_contains(block_node_t *head, block_node_t *node) { //recorre en busqueda de un nodo
    for (block_node_t *c = head; c; c = c->next)
        if (c == node) return 1;
    return 0;
}


void initialize_memory_manager(void) {
    for (int i = 0; i <= MAX_ORDER; i++)
        free_lists[i] = NULL;

    uint32_t ram_mb          = *((uint32_t *)PURE64_RAM_INFO);
    uint64_t total_ram_bytes = (uint64_t)ram_mb * KB_BYTES * KB_BYTES;

    uint64_t max_block_size = PAGE_SIZE * (1ULL << MAX_ORDER); 
    //1ull es 1 as unsiged long long sirve para que por mas que cambiemos max_order siempre tengamos el mayor tamanio de bloque

    uint64_t aligned_start  = (MANAGED_MEMORY_START + max_block_size - 1) & ~(max_block_size - 1); 
    //alinea la memoria managed_memory_start al multiplo de max_block_size mas cercano hacia arriba para poder utilizar xor y encontrar los buddys de la memoria

    buddy_base = (uint8_t *) aligned_start;  //puntero del mmbuddy
    uint8_t  *cursor       = buddy_base; 
    // apunta al proximo byte libre para colocar el siguiente bloque
    uint64_t  remaining = total_ram_bytes - aligned_start; 
    free_bytes = 0;

    //asigna bloques de maximo tamanio, hasta llenar toda la ram o que no haya un orden menor, es para no tener memoria inutil
    for (int order = MAX_ORDER; order >= 0; order--) {
        uint64_t block_size = PAGE_SIZE * (1ULL << order);
        while (remaining >= block_size) {
            list_push(&free_lists[order], (block_node_t *)cursor);
            free_bytes += block_size;
            cursor        += block_size;
            remaining  -= block_size;
        }
    }

    total_manageable_bytes = free_bytes;
}

// saca un bloque del orden pedido. si no hay, parte uno mas grande.
// devuelve el bloque crudo (sin tocar metadata) o NULL si no entra.
static block_node_t *get_block(int order) {
    if (order > MAX_ORDER) return NULL;

    int found = order; // busco la primera lista de orden >= al pedido que tenga algo
    while (found <= MAX_ORDER && free_lists[found] == NULL)
        found++;

    if (found > MAX_ORDER) return NULL;

    block_node_t *block = free_lists[found];
    list_remove(&free_lists[found], block); //retiro el bloque de la lista para asignarlo

    while (found > order) { //particiono hasta llegar al orden que quiero
        found--;
        uint64_t      half  = PAGE_SIZE * (1ULL << found);
        block_node_t *buddy = (block_node_t *)((uint8_t *)block + half);
        list_push(&free_lists[found], buddy);
    }

    free_bytes -= PAGE_SIZE * (1ULL << order);
    return block;
}

// libera un bloque de orden conocido y lo fusiona con su buddy mientras pueda
static void put_block(uint8_t *actual, uint64_t order) {
    if (order > MAX_ORDER) return;

    free_bytes += PAGE_SIZE * (1ULL << order); //agrego el tamanio del bloque liberado

    // fusiono el bloque con su buddy, subo orden mientras pueda
    while (order < MAX_ORDER) {
        uint64_t      block_size = PAGE_SIZE * (1ULL << order);
        uint64_t      relative   = (uint64_t)actual - (uint64_t)buddy_base;
        uint64_t      buddy_rel  = relative ^ block_size; // xor me permite saber donde esta el hermano del bloque para unirlos
        block_node_t *buddy      = (block_node_t *)(buddy_base + buddy_rel);

        if (!list_contains(free_lists[order], buddy))
            break; // el buddy no esta entero libre, no puedo fusionar

        list_remove(&free_lists[order], buddy);

        if ((uint8_t *)buddy < actual) // el bloque fusionado arranca en el menor de los dos
            actual = (uint8_t *)buddy;

        order++;
    }

    list_push(&free_lists[order], (block_node_t *)actual);
}


void* allocate_page(void) {
    return (void*)get_block(0);
}

void free_page(void* ptr) {
    if (ptr == NULL) return;
    put_block((uint8_t*)ptr, 0);
}

// Reserva un bloque de memoria de `bytes` bytes.
// Calcula el orden necesario según el tamaño solicitado,
// obtiene un bloque del buddy system (partiendo bloques más grandes si es necesario),
// almacena la metadata del orden al inicio del bloque, y devuelve un puntero al contenido.
// También maneja la asignación de páginas individuales (bytes <= PAGE_SIZE).
void *allocate_block(uint64_t bytes) {
    if (bytes == 0) return NULL;

    //bytes pedidos + 8 bytes de metadatos para guardar el orden
    uint64_t needed = bytes + sizeof(uint64_t);

    int order = 0; //calculo el orden de bloque segun los bytes que me pidan
    while (order <= MAX_ORDER && (PAGE_SIZE * (1ULL << order)) < needed)
        order++;

    block_node_t *block = get_block(order);
    if (block == NULL) return NULL;

    *((uint64_t *)block) = (uint64_t)order; //orden de bloque a retornar
    return (void *)((uint8_t *)block + sizeof(uint64_t));
}

// Libera un bloque previamente asignado con allocate_block.
// Recupera el orden almacenado en la metadata y delega a put_block
// para devolver el bloque al buddy system, fusionando con su buddy mientras sea posible.
// Funciona tanto para bloques individuales (páginas) como para bloques mayores.
void free_block(void *ptr) {
    if (ptr == NULL) return;

    uint8_t  *actual = (uint8_t *)ptr - sizeof(uint64_t);
    uint64_t  order  = *((uint64_t *)actual); // recupero el orden de la metadata

    put_block(actual, order);
}


void get_mem_stats(mem_info_t *out) {
    if (out == NULL) return;
    out->total_bytes = total_manageable_bytes;
    out->free_bytes  = free_bytes;
    out->used_bytes  = total_manageable_bytes - free_bytes;
}