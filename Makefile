COMMON_FLAGS=-Wall -Wextra -Wno-sign-compare -O3 -march=native -flto -pthread -g
CCFLAGS=$(COMMON_FLAGS) -std=c17
CXXFLAGS=$(COMMON_FLAGS) -std=c++20
LDLIBS=-lpthread -lm -lprofiler

COMMON_OBJS=accessors.o codec.o parse-int.o perms.o board.o chunks.o search.o
BINARIES=backpropagate-losses count-bits count-r1 count-unreachable combine-bitmaps encode-delta integrate-wins lookup-rN minimax print-perm solve-r0 solve-r1 solve-r1-chunked solve-rN solve-lost verify-r0 verify-r1 verify-rN print-r1
TESTS=perms_test search_test ternary_test

all: $(BINARIES) $(TESTS)

perms.o: perms.h perms.cc
	$(CXX) $(CXXFLAGS) -c perms.cc

perms_test: perms_test.cc perms.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

accessors.o: accessors.h accessors.cc chunks.h ternary.h
	$(CXX) $(CXXFLAGS) -c accessors.cc

board.o: board.h board.cc perms.o
	$(CXX) $(CXXFLAGS) -c board.cc

chunks.o: chunks.h chunks.cc board.o
	$(CXX) $(CXXFLAGS) -c chunks.cc

parse-int.o: parse-int.h parse-int.cc
	$(CXX) $(CXXFLAGS) -c parse-int.cc

search.o: search.h search.cc perms.o board.o
	$(CXX) $(CXXFLAGS) -c search.cc

search_test: search_test.cc board.o perms.o search.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

ternary_test: ternary_test.cc ternary.h
	$(CXX) $(CXXFLAGS) -o $@ ternary_test.cc

backpropagate-losses: backpropagate-losses.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

integrate-wins: integrate-wins.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

combine-bitmaps: combine-bitmaps.c
	$(CC) $(CCFLAGS) -o $@ $^ $(LDLIBS)

encode-delta: encode-delta.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

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

lookup-rN: lookup-rN.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

minimax: minimax.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

print-perm: print-perm.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

print-r1: print-r1.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

count-r1: count-r1.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

count-bits: count-bits.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

count-unreachable: count-unreachable.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

verify-r0: verify-r0.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

verify-r1: verify-r1.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

verify-rN: verify-rN.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

test: $(TESTS)
	./perms_test
	./search_test
	./ternary_test

clean:
	rm -f $(BINARIES) $(TESTS) *.o
