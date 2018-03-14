CC=gcc
CXX=g++
CFLAGS=-g3 -Wall -std=gnu11
CPPFLAGS=-g3 -Wall -std=c++11

all:
	$(CC)	main.c mevel.c queue.c -o mainc $(CFLAGS)
	$(CXX)	main.cxx mevel.cpp -o maincxx $(CPPFLAGS)
clean:
	rm -f mainc
	rm -f maincxx
