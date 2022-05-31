COMMON_FLAGS=-Wall -Wextra -Wno-sign-compare -O3 -march=native -flto -pthread
CCFLAGS=$(COMMON_FLAGS) -std=c17
CXXFLAGS=$(COMMON_FLAGS) -std=c++20
LDLIBS=-lpthread -lm -lcrypto -lz

COMMON_OBJS=accessors.o codec.o flags.o parse-int.o perms.o board.o chunks.o search.o
CLIENT_OBJS=client/codec.o client/compress.o client/client.cc client/socket.o client/socket_codec.o
BINARIES=backpropagate-losses count-bits count-r1 count-unreachable combine-bitmaps decode-delta encode-delta integrate-wins lookup-rN minimax print-perm solve-r0 solve-r1 solve-rN solve-lost verify-r0 verify-r1 verify-rN print-r1 client/test-client
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

codec.o: codec.h codec.cc board.o
	$(CXX) $(CXXFLAGS) -c codec.cc

chunks.o: chunks.h chunks.cc board.o
	$(CXX) $(CXXFLAGS) -c chunks.cc

flags.o: flags.h flags.cc
	$(CXX) $(CXXFLAGS) -c flags.cc

parse-int.o: parse-int.h parse-int.cc
	$(CXX) $(CXXFLAGS) -c parse-int.cc

search.o: search.h search.cc perms.o board.o
	$(CXX) $(CXXFLAGS) -c search.cc

search_test: search_test.cc board.o perms.o search.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

ternary_test: ternary_test.cc ternary.h
	$(CXX) $(CXXFLAGS) -o $@ ternary_test.cc

client/client.o: client/client.h client/client.cc  client/error.h client/bytes.h client/codec.o client/compress.o client/socket.o client/socket_codec.o
	$(CXX) $(CXXFLAGS) -o $@ -c client/client.cc

# Must use -o $@ otherwise the object file gets written in the
# current directory instead of in the client subdirectory!
client/codec.o: client/codec.h client/codec.cc client/bytes.h client/byte_span.h
	$(CXX) $(CXXFLAGS) -o $@ -c client/codec.cc

client/compress.o: client/compress.h client/compress.cc client/bytes.h client/byte_span.h
	$(CXX) $(CXXFLAGS) -o $@ -c client/compress.cc

client/socket.o: client/socket.h client/socket.cc
	$(CXX) $(CXXFLAGS) -o $@ -c client/socket.cc

client/socket_codec.o: client/socket_codec.h client/socket_codec.cc client/codec.o client/socket.o
	$(CXX) $(CXXFLAGS) -o $@ -c client/socket_codec.cc

backpropagate-losses: backpropagate-losses.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

integrate-wins: integrate-wins.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

combine-bitmaps: combine-bitmaps.c
	$(CC) $(CCFLAGS) -o $@ $^ $(LDLIBS)

decode-delta: decode-delta.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

encode-delta: encode-delta.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

solve-r0: solve-r0.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

solve-r1: solve-r1.cc $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

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

client/test-client: client/test-client.cc $(CLIENT_OBJS) flags.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

test: $(TESTS)
	./perms_test
	./search_test
	./ternary_test

clean:
	rm -f $(BINARIES) $(TESTS) *.o client/*.o
