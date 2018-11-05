CFLAGS := -ffreestanding -nostdinc -O3
OBJECTS = main.o

all: init

init: $(OBJECTS)
	ld $(OBJECTS) -o $@

%.o: %.c
	gcc $< -c $(CFLAGS) -o $@

clean:
	rm -f $(OBJECTS)
	rm -f init

.PHONY: clean
