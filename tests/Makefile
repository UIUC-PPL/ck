INCLUDES:=$(INCLUDES) -I../include
OPTS:=$(OPTS) -c++-option -std=c++17 $(INCLUDES)

all: hello.out histogram.out

hello.out: hello.cpp
	charmc $(OPTS) hello.cpp -o hello.out

histogram.out: histogram.cpp histogram.hpp
	charmc $(OPTS) histogram.cpp -o histogram.out

clean:
	rm -f charmrun hello.out histogram.out
