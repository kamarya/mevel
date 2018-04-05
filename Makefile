CC=gcc
CXX=g++
CFLAGS=-g3 -Wall -std=gnu11 -I.
CPPFLAGS=-g3 -Wall -std=c++11 -I.

SRCS = mevel.c queue.c
OBJS := ${SRCS:.c=.o}

.PHONY: all

all: $(OBJS)
	ar -rcs libmevel.a $(OBJS)
	rm -f $(OBJS)

example:
	$(CC)	example/main.c mevel.c queue.c -o mainc $(CFLAGS)
	$(CXX)	example/main.cxx mevel.cpp -o maincxx $(CPPFLAGS)

clean:
	rm -f mainc
	rm -f maincxx
