COMMON_FLAGS=-Wall -Wextra -Wno-sign-compare -O3 -march=native -flto -pthread
CFLAGS=$(COMMON_FLAGS) -std=c17
CXXFLAGS=$(COMMON_FLAGS) -std=c++20
LDFLAGS=
LDLIBS=-lpthread -lm
CLIENT_LDLIBS=-lcrypto -lz

COMMON_OBJS=$(addprefix $(OBJDIR)/,accessors.o codec.o flags.o parse-int.o perms.o board.o chunks.o search.o)
CLIENT_OBJS=$(addprefix $(OBJDIR)/client/,codec.o compress.o client.o socket.o socket_codec.o)
BINARIES=backpropagate-losses count-bits count-r1 count-unreachable combine-bitmaps decode-delta encode-delta integrate-wins lookup-rN minimax print-perm solve-r0 solve-r1 solve-rN solve-lost verify-r0 verify-r1 verify-rN print-r1 client/test-client
TESTS=perms_test search_test ternary_test

C_SRCS = $(wildcard *.c)
CXX_SRCS = $(wildcard *.cc client/*.cc)

all: $(BINARIES) $(TESTS)

test: $(TESTS)
	./perms_test
	./search_test
	./ternary_test

clean:
	rm -f $(BINARIES) $(TESTS)
	rm -f -R $(DEPDIR) $(OBJDIR)

.PHONY: all test clean

# Disable default rules (which makes it easier to debug Makefile problems)
.SUFFIXES:

# Logic for automatic dependency generation below has been borrowed from:
# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

DEPDIR := deps
OBJDIR := objs

$(OBJDIR)/%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)/ $(OBJDIR)/
	$(CC) -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o : %.cc $(DEPDIR)/%.d | $(DEPDIR)/ $(OBJDIR)/
	$(CXX) -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d  $(CXXFLAGS) -c -o $@ $<
 
$(OBJDIR)/client/%.o : client/%.cc $(DEPDIR)/client/%.d | $(DEPDIR)/client/ $(OBJDIR)/client/
	$(CXX) -MT $@ -MMD -MP -MF $(DEPDIR)/client/$*.d $(CXXFLAGS) -c -o $@ $<
 
deps/: ; mkdir -p $@
deps/client/: ; mkdir -p $@
objs/: ; mkdir -p $@
objs/client/: ; mkdir -p $@

DEPFILES := $(C_SRCS:%.c=$(DEPDIR)/%.d) $(CXX_SRCS:%.cc=$(DEPDIR)/%.d)

$(DEPFILES):

include $(wildcard $(DEPFILES))

# Rules to build tests follow.

perms_test: $(OBJDIR)/perms_test.o $(OBJDIR)/perms.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

search_test: $(OBJDIR)/search_test.o $(OBJDIR)/search.o $(OBJDIR)/board.o $(OBJDIR)/perms.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

ternary_test: $(OBJDIR)/ternary_test.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Rules to build binaries follow.

backpropagate-losses: $(OBJDIR)/backpropagate-losses.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

count-bits: $(OBJDIR)/count-bits.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

count-r1: $(OBJDIR)/count-r1.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

count-unreachable: $(OBJDIR)/count-unreachable.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

combine-bitmaps: $(OBJDIR)/combine-bitmaps.o
	$(CC) $(CCFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

decode-delta: $(OBJDIR)/decode-delta.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

encode-delta: $(OBJDIR)/encode-delta.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

integrate-wins: $(OBJDIR)/integrate-wins.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

lookup-rN: $(OBJDIR)/lookup-rN.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

minimax: $(OBJDIR)/minimax.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

print-perm: $(OBJDIR)/print-perm.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

solve-r0: $(OBJDIR)/solve-r0.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

solve-r1: $(OBJDIR)/solve-r1.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

solve-rN: $(OBJDIR)/solve-rN.o $(COMMON_OBJS) $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(CLIENT_LDLIBS)

solve-lost: $(OBJDIR)/solve-lost.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

verify-r0: $(OBJDIR)/verify-r0.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

verify-r1: $(OBJDIR)/verify-r1.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

verify-rN: $(OBJDIR)/verify-rN.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

print-r1: $(OBJDIR)/print-r1.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

client/test-client: $(OBJDIR)/client/test-client.o $(COMMON_OBJS) $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(CLIENT_LDLIBS)

# EOF
