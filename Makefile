CXX=~/mos-bin/mos-cx16-clang++
CXXFLAGS=-Os -g -fno-common -isystem .
SRCS=$(wildcard *.cc)
OBJS=$(SRCS:.cc=.o)

chibicc: $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): chibicc.h

test: chibicc
	./test.sh

clean:
	rm -f chibicc *.o *~ tmp*

.PHONY: test clean
