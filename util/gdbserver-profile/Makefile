PROGRAM := gdbserver-profiler
OBJECTS := main.o image_reader.o gdbserver_handler.o trace_writer.o

all: $(PROGRAM)

%.o: %.c
	cc -c -o $@ $< -Wall -Werror $(CFLAGS)

$(PROGRAM): $(OBJECTS)
	cc -o $@ $(OBJECTS)

clean:
	rm -f $(OBJECTS) $(PROGRAM)

.PHONY: clean
