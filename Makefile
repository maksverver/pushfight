include Makefile.defs

all: $(ALL_BINARIES) $(TESTS)

test: $(TESTS)
	./perms_test
	./search_test
	./ternary_test

clean:
	rm -f $(BINARIES) $(TESTS)
	rm -f -R $(DEPDIR) $(OBJDIR)

.PHONY: all test clean

include Makefile.rules
