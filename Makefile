CXXFLAGS=-std=c++20 -Wall -Wextra -Wno-sign-compare -O3 -march=native -g

BINARIES=countbits solver print
TESTS=perms_test

all: $(BINARIES) $(TESTS)

perms.o: perms.h perms.cc
	$(CXX) $(CXXFLAGS) -c perms.cc

perms_test: perms_test.cc perms.o
	$(CXX) $(CXXFLAGS) -o $@ $^

board.o: board.h board.cc perms.o
	$(CXX) $(CXXFLAGS) -c board.cc

search.o: search.h search.cc perms.o board.o
	$(CXX) $(CXXFLAGS) -c search.cc

solver: solver.cc board.o perms.o search.o
	$(CXX) $(CXXFLAGS) -o $@ $^

print: print.cc board.o perms.o search.o
	$(CXX) $(CXXFLAGS) -o $@ $^

countbits: countbits.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TESTS)
	./perms_test

clean:
	rm -f $(BINARIES) $(TESTS) *.o
