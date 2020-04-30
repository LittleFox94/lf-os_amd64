CFLAGS := -g -nostdlib -I ../../sysroot/include -ffreestanding $(OPTIMIZATION)
OBJECTS = main.o

all: init

init: $(OBJECTS) ../../sysroot/lib/libc.a ../../sysroot/lib/crt0.o
	ld.lld $(OBJECTS) ../../sysroot/lib/crt0.o ../../sysroot/lib/libc.a -o $@

%.o: %.c ../../sysroot/include
	clang $< -c $(CFLAGS) -o $@

clean:
	rm -f $(OBJECTS)
	rm -f init

.PHONY: clean
