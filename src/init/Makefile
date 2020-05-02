CFLAGS := -g --sysroot ../../sysroot $(OPTIMIZATION)
OBJECTS = main.o

all: init

init: $(OBJECTS) ../../sysroot/lib/libc.a ../../sysroot/lib/crt0.o
	clang -fuse-ld=lld -rtlib=compiler-rt $(CFLAGS) $(OBJECTS) -o $@

%.o: %.c ../../sysroot/include
	clang $< -c $(CFLAGS) -o $@

clean:
	rm -f $(OBJECTS)
	rm -f init

.PHONY: clean
