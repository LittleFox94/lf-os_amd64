TESTS = slab string tpa

all: $(addprefix run_,$(TESTS))

run_%: %.t
	./$<

%.t: %.c lfostest.h
	clang -g -o $@ $< -I../../src/kernel/ -I.

clean:
	rm -f $(addsuffix .t,$(TESTS))

.PHONY: clean
