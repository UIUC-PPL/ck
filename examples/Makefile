INCLUDES:=$(INCLUDES) -I../include
OPTS:=$(OPTS) -c++-option -std=c++17 $(INCLUDES)

all: fib.out hello.out hierarchical_hello.out histogram.out leanmd.out

fib.out: fib.cpp
	charmc $(OPTS) fib.cpp -o fib.out

hello.out: hello.cpp
	charmc $(OPTS) hello.cpp -o hello.out

hierarchical_hello.out: hierarchical_hello.cpp
	charmc $(OPTS) hierarchical_hello.cpp -o hierarchical_hello.out -module CommonLBs

histogram.out: histogram.cpp histogram.hpp
	charmc $(OPTS) histogram.cpp -o histogram.out

leanmd.out: leanmd/*.*pp
	charmc $(OPTS) -Ileanmd/ leanmd/leanmd.cpp -o leanmd.out

clean:
	rm -f *.log.gz *.out.projrc *.out.sts
	rm -f charmrun fib.out hello.out hierarchical_hello.out histogram.out leanmd.out
