TESTS = slab string

all: $(addprefix run_,$(TESTS))

run_%: %.t
	./$<

%.t: %.c lfostest.h
	clang -o $@ $< -I../../src/kernel/ -I.

clean:
	rm -f $(addsuffix .t,$(TESTS))

.PHONY: clean
