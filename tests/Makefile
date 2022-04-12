INCLUDES:=$(INCLUDES) -I../include
OPTS:=$(OPTS) -c++-option -std=c++17 $(INCLUDES)

all: hello.out

hello.out: hello.cpp
	charmc $(OPTS) hello.cpp -o hello.out

clean:
	rm -f charmrun hello.out
