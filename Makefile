CXXFLAGS=-std=c++20 -Wall -Wextra -Wno-sign-compare -O3 -march=native -flto -pthread -lpthread -lm

COMMON_OBJS=perms.o board.o search.o
BINARIES=countbits print solve-r0 verify-r0
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

solve-r0: solve-r0.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

print: print.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

countbits: countbits.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

verify-r0: verify-r0.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TESTS)
	./perms_test

clean:
	rm -f $(BINARIES) $(TESTS) *.o
