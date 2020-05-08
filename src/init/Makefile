OBJECTS = main.o

all: init

init: $(OBJECTS)
	$(CXX) $(OPTIMIZATION) -static $(CXXFLAGS) $(OBJECTS) -o $@

%.o: %.cxx
	$(CXX) $(OPTIMIZATION) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -f init

.PHONY: clean
