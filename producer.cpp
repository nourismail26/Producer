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
#include <buffer.h>
#define shm_key 1234
#define ERROR -1

struct commodity  {
    std::string name;
    double price;
};
double generate_price(double mean, double stddev){
    std::default_random_engine generator;
    std::normal_distribution<double> dist(mean, stddev);
    return dist(generator);
}

commodity produce(std::string Name , double mean , double stddev){
    commodity c ;
    c.name = Name;
    c.price = generate_price(mean , stddev);
    return c;

}
bool add_to_buffer(commodity c){
    return false;
}

int get_shared_buffer(int size){ //returns the buffer id
    key_t key = shm_key;
    int buffer_id = shmget(key,size,0666 | IPC_CREAT); 
        //key -> unique & identified by the user 
        //buffer_id -> unique, returned by shmget & assigned by os  
    return buffer_id;
}

buffer* attach_buffer(int size){    //attach the producer to buffer
    int shared_buffer_id = get_shared_buffer(size);
    buffer *result;

    if (shared_buffer_id == ERROR){
        return NULL;
    }
    result = (buffer*) shmat(shared_buffer_id, NULL,0 );
    if (result == (buffer *)ERROR){
        return NULL;
    }
    return result;

}
int main(int argc, char* argv[]) {

    //checking for the number of arguments
    if (argc < 6) {
        std::cerr << "Usage: <program_name> <commodity> <mean> <stddev> <sleep interval> <size of buffer>\n";
        return 1;
    }

    std::string commodity_name = argv[1];
    double mean = std::stod(argv[2]);
    double stddev = std::stod(argv[3]);
    int sleepInterval = std::stoi(argv[4]); // in milliseconds
    int bufferSize = std::stoi(argv[5]);
    
      //print the values of the variables
    std::cout << "Commodity Name: " << commodity_name << std::endl;
    std::cout << "Mean: " << mean << std::endl;
    std::cout << "Standard Deviation: " << stddev << std::endl;
    std::cout << "Sleep Interval: " << sleepInterval << " ms" << std::endl;
    std::cout << "Buffer Size: " << bufferSize << std::endl;

     if (bufferSize <= 0) {
        std::cerr << "Buffer size must be a positive integer.\n";
        return 1;
    }
     if (commodity_name.length() >= 10) {
        std::cerr << "Error: Commodity name must be shorter than 10 characters.\n";
        return 1;
    }
  

   //INITIALIZING THE SEMAPHORES
    sem_t *e = sem_open("empty_sem", O_CREAT, 0666, bufferSize);
    sem_t *n = sem_open("full_sem", O_CREAT, 0666, 0);
    sem_t *mutex = sem_open("mutex_sem", O_CREAT, 0666, 1);

 while (true) {

        commodity c = produce(commodity_name,mean,stddev);
        sem_wait(e);      
        sem_wait(mutex);  
        add_to_buffer(c);
        sem_post(mutex);
        sem_post(n); 
    }

    ///shmdt(mybuffer);
    sem_close(e);
    sem_close(n);
    sem_close(mutex);

 return 0;
}

