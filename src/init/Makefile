OBJECTS = main.o

all: init

init: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

clean:
	rm -f $(OBJECTS)
	rm -f init

.PHONY: clean
