#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <chrono>
#include <unistd.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <cmath>
#include <fcntl.h>
#define sizeofbuffer 40
#define shm_key 1234

struct buffer {
    char name[10];
    double prices[sizeofbuffer];
    int index;
};
int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: <program_name> <commodity> <mean> <stddev> <sleep interval> <size of buffer>\n";
        return 1;
    }

    std::string commodity = argv[1];
    double mean = std::stod(argv[2]);
    double stddev = std::stod(argv[3]);
    int sleepInterval = std::stoi(argv[4]); // in milliseconds
    int bufferSize = std::stoi(argv[5]);

     if (bufferSize <= 0) {
        std::cerr << "Buffer size must be a positive integer.\n";
        return 1;
    }
     if (commodity.length() >= 10) {
        std::cerr << "Error: Commodity name must be shorter than 10 characters.\n";
        return 1;
    }
    if (bufferSize > sizeofbuffer) {
        std::cerr << "Buffer size exceeds the maximum limit (" << sizeofbuffer << ").\n";
        return 1;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dist(mean, stddev);

    //return indentifier for the shared memory 
    //key =1234
    int shm_id = shmget(shm_key, sizeof(buffer) , 0666 | IPC_CREAT);
    if (shm_id < 0) {
        std::cerr << "Error: Shared Memory identifier must be greater than 0.\n";
        return 1;
    }


    //now we have to attach my buffer to the shared memory
    buffer *mybuffer = (buffer *)shmat(shm_id, nullptr, 0);
    if (mybuffer == (void *)-1) {
        std::cerr << "Error: Can not attach buffer to shared memory.\n";
        return 1;
    }

    sem_t *e = sem_open("empty_sem", O_CREAT, 0666, bufferSize);
    // ana msh 3arfa heya el mafrood teb2a sizeofbuffer ely howa 40 wala buffer size
    // msh 3arfa hatefre2 wala la
    sem_t *n = sem_open("full_sem", O_CREAT, 0666, 0);
    sem_t *mutex = sem_open("mutex_sem", O_CREAT, 0666, 1);

 while (true) {
        
        double price = dist(gen);
        sem_wait(e);  
        sem_wait(mutex);  

        
        mybuffer->prices[mybuffer->index] = price;
        mybuffer->index = (mybuffer->index + 1) % bufferSize;

        sem_post(mutex);
        sem_post(n); 

        
    }
    shmdt(mybuffer);
    sem_close(e);
    sem_close(n);
    sem_close(mutex);
 return 0;
}

