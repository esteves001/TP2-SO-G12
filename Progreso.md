# Registro de Progreso

## 1. Limpieza del proyecto
Se limpió toda la estructura antigua que se tenía de Arqui (sonidos, juego, etc.).

## 2. Configuración del Build
Nos aseguramos de que el proceso de compilación (*building*) sea correcto.

## 3. Investigación: Memory Manager
Se investigó sobre qué *Memory Manager* podíamos implementar. 

**Respuesta de la IA:**
> "El Bitmap es el compañero ideal para el Buddy System. El Buddy System es genial para fragmentación controlada, pero es complejo. El Bitmap es lineal, predecible, elegante a nivel de bits y sumamente fácil de debugear (puedes imprimir el entero en binario en la consola y ver literalmente el estado de tu memoria RAM). Esta es la forma clásica en la que Linux y otros sistemas operativos reales administran la memoria física base."

* **Cómo funciona:** Se crea un arreglo enorme de bits. Cada bit representa un marco de pagina. Si el bit es 0, el marco está libre; si es 1, está ocupado. Usando uint8_t dividimos dicha memoria en cajas de 1 Byte (8 Bits). Donde podremos acceder a ellas usando []

bitmap[0] -> Contiene las los marcos de paginas 0 al 7
bitmap[1] -> Contiene las los marcos de paginas 8 al 15

uint64_t / 8 -> Byte(fila) 
uint8_t % 8 -> Bit(columna)

* **Eficiencia:** Es ridículamente eficiente en memoria. Para rastrear 1 GB de RAM (marcos de 4KB), el bitmap solo ocupa 32 KB.

## 4. Procesos
Se agregaron los archivos process.c y process.h en el .h se encuentra la estructura principal de un proceso, se tiene por lo pronto un array estatico con todos los procesos en cualquier estado.
Tambien se agrego el enum de estados y se agrega el estado de muerto para saber cuando retirarlo del array de procesos.
Funcionalidades crear, eliminar, bloquear y desbloquear un proceso.
Scheduler basico pero funcional por lo pronto.

## 5. Pipes
En process.h se adiciono la estructura del pipe y funciones de pipes (escritura, lectura, cerrar y crear). 
Por lo pronto un pipe tiene un buffer no tan grande respecto a la pagina, se puede modificar.
Tambien se modificaron funciones anteriores para que se adapten a los pipes.
