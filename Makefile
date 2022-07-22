include Makefile.defs

COMMON_FLAGS += -march=native

all: $(ALL_BINARIES) $(TESTS)

test: $(TESTS)
	./efcodec_test
	./perms_test
	./search_test
	./ternary_test

clean:
	rm -f -R $(DEPDIR) $(OBJDIR)

distclean: clean
	rm -f $(ALL_BINARIES) $(TESTS)

.PHONY: all test clean

include Makefile.rules
