include Makefile.defs

COMMON_FLAGS += -march=native

all: $(ALL_BINARIES) $(TESTS)

test: $(TESTS)
	./efcodec_test
	./perms_test
	./search_test
	./ternary_test

clean:
	rm -f $(BINARIES) $(TESTS)
	rm -f -R $(DEPDIR) $(OBJDIR)

.PHONY: all test clean

include Makefile.rules
