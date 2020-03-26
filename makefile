CC=g++
CFLAGS=-std=c++1z
example: example.cpp
	$(CC) -o out example.cpp $(CFLAGS) -lpulse

