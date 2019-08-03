CFLAGS := -ffreestanding -nostdinc -O3
OBJECTS = main.o

all: init

init: $(OBJECTS)
	ld.lld $(OBJECTS) -o $@

%.o: %.c
	clang $< -c $(CFLAGS) -o $@

clean:
	rm -f $(OBJECTS)
	rm -f init

.PHONY: clean
