CXXFLAGS=-std=c++20 -Wall -Wextra -Wno-sign-compare -O3 -march=native -g

BINARIES=solver
TESTS=perms_test

all: $(BINARIES) $(TESTS)

perms.o: perms.h perms.cc
	$(CXX) $(CXXFLAGS) -c perms.cc

perms_test: perms_test.cc perms.o
	$(CXX) $(CXXFLAGS) -o $@ $^

solver: solver.cc perms.o
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TESTS)
	./perms_test

clean:
	rm -f $(BINARIES) $(TESTS) *.o
