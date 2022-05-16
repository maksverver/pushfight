CXXFLAGS=-std=c++20 -Wall -Wextra -O2 -g

TESTS=perms_test

all: $(TESTS)

perms.o: perms.h perms.cc
	$(CXX) $(CXXFLAGS) -c perms.cc

perms_test: perms_test.cc perms.o
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TESTS)
	./perms_test

clean:
	rm -f $(TESTS) *.o
