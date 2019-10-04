CC=gcc
CXX=g++
CFLAGS=-g3 -Wall -std=gnu11 -I./inc -L.
CPPFLAGS=-g3 -Wall -std=c++11 -I./inc -L.

SRCS = src/mevel.c src/queue.c
OBJS := ${SRCS:.c=.o}

.PHONY: all

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

all: $(OBJS)
	ar -rcs libmevel.a $(OBJS)
	rm -f $(OBJS)

example: all
	$(CC)	example/main.c -o mainc -lmevel $(CFLAGS)
	$(CXX)	example/main.cxx src/mevel.cpp -o maincxx -lmevel $(CPPFLAGS)

clean:
	rm -f mainc
	rm -f maincxx
	rm -f libmevel.a

