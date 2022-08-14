CC=gcc
CXX=g++
CFLAGS=-g3 -Wall -std=gnu11 -I./inc -L.
CXXFLAGS=-g3 -Wall -std=c++11 -I./inc -L.


.PHONY: all

all:
	$(CC) $(CFLAGS) -c src/mevel.c -o mevel.c.o
	$(CC) $(CFLAGS) -c src/queue.c -o queue.c.o
	$(CXX) $(CXXFLAGS) -c src/mevel.cpp -o mevel.cpp.o
	ar -rcs libmevel.a mevel.c.o queue.c.o mevel.cpp.o

example: all
	$(CC)	example/main.c -o mainc -lmevel $(CFLAGS)
	$(CXX)	example/main.cxx -o maincxx -lmevel $(CXXFLAGS)

clean:
	rm -f mainc
	rm -f maincxx
	rm -f libmevel.a
	rm -f mevel.c.o queue.c.o mevel.cpp.o
