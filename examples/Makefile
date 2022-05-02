INCLUDES:=$(INCLUDES) -I../include
OPTS:=$(OPTS) -c++-option -std=c++17 $(INCLUDES)

all: fib.out hello.out hierarchical_hello.out histogram.out leanmd.out

common.o: ../include/ck/common.cpp
	charmc $(OPTS) ../include/ck/common.cpp -c -o common.o

fib.out: fib.cpp common.o
	charmc $(OPTS) fib.cpp common.o -o fib.out

hello.out: hello.cpp common.o
	charmc $(OPTS) hello.cpp common.o -o hello.out

hierarchical_hello.out: hierarchical_hello.cpp common.o
	charmc $(OPTS) hierarchical_hello.cpp common.o -o hierarchical_hello.out -module CommonLBs

histogram.out: histogram.cpp histogram.hpp common.o
	charmc $(OPTS) histogram.cpp common.o -o histogram.out

leanmd.out: leanmd/*.*pp common.o
	charmc $(OPTS) -Ileanmd/ leanmd/leanmd.cpp common.o -o leanmd.out

clean:
	rm -f *.log.gz *.out.projrc *.out.sts common.o
	rm -f charmrun fib.out hello.out hierarchical_hello.out histogram.out leanmd.out
