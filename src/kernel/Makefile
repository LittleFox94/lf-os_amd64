OBJECTS = fbconsole.o string.o sd.o elf.o version.o slab.o log.o tpa.o

liblfos-kernel.a: version.o $(OBJECTS)
	ar rcs $@ $(OBJECTS)

version.c:
	rm -f version.o

%.o: %.c config.h
	$(CC) $< -c $(CFLAGS) -o $@

clean:
	+ make -C arch/amd64 clean
	rm -f $(OBJECTS)
	rm -f liblfos-kernel.a

.PHONY: all clean version.c liblfos-kernel.a
