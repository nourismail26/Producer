# Use GNU compiler
CC = g++ -g

all: producer consumer

producer.o: producer.cpp
	$(CC) -c producer.cpp

producer: producer.o
	$(CC) -o producer producer.o

consumer.o: consumer.cpp
	$(CC) -c consumer.cpp

consumer: consumer.o
	$(CC) -o consumer consumer.o

clean:
	rm -f producer *.o consumer *.o
