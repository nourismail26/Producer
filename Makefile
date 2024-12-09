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
	
# Run the producer executable with arguments
producer: producernew
	@./producernew $(ARGS)
 
# Run the consumer executable with one argument
consumer: consumernew
	@./consumernew $(ARG)
 
# Clean the build files
clean:
	rm -f producernew consumernew consumernew.o producernew.o
