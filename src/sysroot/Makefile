DIRS = bin include lib src

src_dir := $(dir $(realpath $(firstword $(MAKEFILE_LIST))))

all: $(DIRS) $(shell find $(src_dir)static -type f -and -not -name '.*' -printf "%p")
	rsync -ar $(src_dir)static/ .

include: src
lib:     include

%: $(realpath $(firstword $(MAKEFILE_LIST))).%
	mkdir -p $@
	+ make -C $@ -f $<

clean:
	mkdir -p $(DIRS)
	+ make -C bin 	  -f $(realpath $(firstword $(MAKEFILE_LIST))).bin 		clean
	+ make -C include -f $(realpath $(firstword $(MAKEFILE_LIST))).include 	clean
	+ make -C lib     -f $(realpath $(firstword $(MAKEFILE_LIST))).lib 		clean
	+ make -C src     -f $(realpath $(firstword $(MAKEFILE_LIST))).src 		clean

.PHONY: clean
