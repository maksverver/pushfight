CXXFLAGS=-std=c++20 -Wall -Wextra -Wno-sign-compare -O3 -march=native -flto -pthread
LDLIBS=-lpthread -lm

COMMON_OBJS=accessors.o perms.o board.o chunks.o search.o
BINARIES=count-bits count-unreachable minimax print-perm solve-r0 solve-r1 solve-r1-chunked solve-rN solve-lost verify-r0 print-r1
TESTS=perms_test

all: $(BINARIES) $(TESTS)

perms.o: perms.h perms.cc
	$(CXX) $(CXXFLAGS) -c perms.cc

perms_test: perms_test.cc perms.o
	$(CXX) $(CXXFLAGS) -o $@ $^

accessors.o: accessors.h accessors.cc
	$(CXX) $(CXXFLAGS) -c accessors.cc

board.o: board.h board.cc perms.o
	$(CXX) $(CXXFLAGS) -c board.cc

chunks.o: chunks.h chunks.cc board.o
	$(CXX) $(CXXFLAGS) -c chunks.cc

search.o: search.h search.cc perms.o board.o
	$(CXX) $(CXXFLAGS) -c search.cc

solve-r0: solve-r0.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

solve-r1: solve-r1.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

solve-r1-chunked: solve-r1.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -DCHUNKED_ACCESSOR -o $@ $^ $(LDLIBS)

solve-rN: solve-rN.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

solve-lost: solve-lost.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

minimax: minimax.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

print-perm: print-perm.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

print-r1: print-r1.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

count-bits: count-bits.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

count-unreachable: count-unreachable.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

verify-r0: verify-r0.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

test: $(TESTS)
	./perms_test

clean:
	rm -f $(BINARIES) $(TESTS) *.o
