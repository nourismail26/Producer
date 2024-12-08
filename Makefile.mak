CC = g++
CFLAGS = -std=c++17 -Wall
LDFLAGS = -pthread

all: producernew consumernew

producernew: producernew.o
	$(CC) -o producernew producernew.o $(LDFLAGS)

consumernew: consumernew.o
	$(CC) -o consumernew consumernew.o $(LDFLAGS)

producernew.o: producernew.cpp buffer.h
	$(CC) $(CFLAGS) -c producernew.cpp

consumernew.o: consumernew.cpp buffer.h
	$(CC) $(CFLAGS) -c consumernew.cpp

clean:
	rm -f *.o producernew consumernew
