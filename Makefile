CC = g++

all: producer consumer
 
producer: 
	$(CC) -o producer producer.cpp

consumer: 
	$(CC) -o consumer consumer.cpp 

 
# Clean the build files
clean:
	rm -f producer consumer consumer.o producer.o
