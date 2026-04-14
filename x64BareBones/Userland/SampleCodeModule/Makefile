include ../Makefile.inc

MODULE     := 0000-sampleCodeModule.bin
LDSCRIPT   := sampleCodeModule.ld

MY_CFLAGS  := $(GCCFLAGS) \
               -Iinclude \
               -IUserLib/include \
               -ITerminal/lib \
               -IPongisGolf/lib

CSOURCES   := $(wildcard [^_]*.c)
COBJECTS   := $(CSOURCES:.c=.o)
ASMSOURCES := $(wildcard *.asm)
ASMOBJS    := $(ASMSOURCES:.asm:.o)
LOADER_O   := _loader.o

.PHONY: all clean UserLib Terminal PongisGolf

all: UserLib Terminal PongisGolf $(MODULE)

UserLib:
	$(MAKE) -C UserLib

Terminal:
	$(MAKE) -C Terminal

PongisGolf:
	$(MAKE) -C PongisGolf

$(LOADER_O): _loader.c
	$(GCC) $(MY_CFLAGS) -c $< -o $@

%.o: %.c
	$(GCC) $(MY_CFLAGS) -c $< -o $@

%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(MODULE): $(LOADER_O) $(COBJECTS) $(ASMOBJS) \
            UserLib/libuserland.a \
            Terminal/libterminal.a \
            PongisGolf/libpongisgolf.a
	$(GCC) $(MY_CFLAGS) -T $(LDSCRIPT) \
          $(LOADER_O) $(COBJECTS) $(ASMOBJS) Terminal/libterminal.a PongisGolf/libpongisgolf.a UserLib/libuserland.a \
          -o ../$(MODULE) $(LDFLAGS)

clean:
	rm -f $(LOADER_O) $(COBJECTS) $(ASMOBJS) ../$(MODULE)
	$(MAKE) -C UserLib clean
	$(MAKE) -C Terminal clean
	$(MAKE) -C PongisGolf clean