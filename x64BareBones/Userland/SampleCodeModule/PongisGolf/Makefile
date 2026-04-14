include ../../Makefile.inc

MY_CFLAGS := $(GCCFLAGS) -Ilib -I../UserLib/include -I../Terminal/lib
CSOURCES  := $(wildcard *.c)
OBJECTS   := $(CSOURCES:.c=.o)
LIB       := libpongisgolf.a

.PHONY: all clean

all: $(LIB)

%.o: %.c
	$(GCC) $(MY_CFLAGS) -c $< -o $@

$(LIB): $(OBJECTS)
	$(AR) rcs $@ $^

clean:
	rm -f $(OBJECTS) $(LIB)
