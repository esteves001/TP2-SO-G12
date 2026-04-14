include ../../Makefile.inc

ASM_SRCS := $(wildcard asm/*.asm)
ASM_OBJS := $(ASM_SRCS:.asm=.o)

C_SRCS   := $(wildcard *.c)
C_OBJS   := $(C_SRCS:.c=.o)

LIB      := libuserland.a

.PHONY: all clean
all: $(LIB)

asm/%.o: asm/%.asm
	$(ASM) $(ASMFLAGS) $< -o $@

%.o: %.c
	$(GCC) $(GCCFLAGS) -Iinclude -c $< -o $@

$(LIB): $(ASM_OBJS) $(C_OBJS)
	$(AR) rcs $@ $^

clean:
	rm -f asm/*.o *.o $(LIB)
