TRACE_OBJECTS := gsp-trace.o image_reader.o gdbserver_handler.o trace_writer.o
SYMS_OBJECTS  := gsp-syms.o  image_reader.o trace_reader.o

all: gsp-trace gsp-syms

%.o: %.c
	cc -c -o $@ $< -Wall -Werror $(CFLAGS)

gsp-trace: $(TRACE_OBJECTS)
	cc -o $@ $(TRACE_OBJECTS)

gsp-syms: $(SYMS_OBJECTS)
	cc -o $@ $(SYMS_OBJECTS)

clean:
	rm -f $(TRACE_OBJECTS) $(SYMS_OBJECTS) gsp-trace gsp-syms

.PHONY: clean
