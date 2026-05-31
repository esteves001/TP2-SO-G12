include Makefile.inc

KERNEL=kernel.bin

# Por defecto, usamos el bitmap memory manager.
MEMORY_MANAGER_SRC ?= bitmapmm.c

# Excluimos los dos administradores de memoria de la lista de fuentes principal.
SOURCES=$(filter-out buddymm.c bitmapmm.c, $(wildcard *.c))
SOURCES_ASM=$(wildcard asm/*.asm)

# Agregamos el administrador de memoria elegido a las fuentes.
SOURCES += $(MEMORY_MANAGER_SRC)

OBJECTS=$(SOURCES:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
LOADERSRC=loader.asm

LOADEROBJECT=$(LOADERSRC:.asm=.o)
STATICLIBS=

all: $(KERNEL)

# El target 'buddy' simplemente vuelve a llamar a make con la variable MEMORY_MANAGER_SRC cambiada.
buddy:
	@$(MAKE) all MEMORY_MANAGER_SRC=buddymm.c

$(KERNEL): $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM)
	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(LOADEROBJECT):
	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

clean:
	rm -rf asm/*.o *.o *.bin

.PHONY: all clean buddy
