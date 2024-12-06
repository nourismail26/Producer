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
#include "buffer.h"
#include <time.h>
#define shm_key 1234
#define ERROR -1

std::string message ;  //global variable -> to track the message to be logged 

double generate_price(double mean, double stddev){
    std::default_random_engine generator;
    std::normal_distribution<double> dist(mean, stddev);
    return dist(generator);
}

commodity produce(std::string Name , double mean , double stddev){
    commodity c ;
    c.name = Name;
    c.price = generate_price(mean , stddev);
    message = "generating a new value of " + std::to_string(c.price) + "\n";
    return c;

}
bool add_to_buffer(commodity c){
    message = "adding a new value to buffer " + std::to_string(c.price) + "\n";
    return false;
}
void logMessage(std::string name){

    timespec timeptr;
    clock_gettime(CLOCK_REALTIME, &timeptr);
    tm *time = localtime(&timeptr.tv_sec);
    char temp[30];
    strftime(temp,sizeof(temp) , "%m/%d/%Y %H:%M:%S" , time);
    std::string space =" ";
    std::string left ="[";
    message =  left+ temp +"]"+ space +name+ space+ message;
    std::cerr << message << std::endl;
}
void sleep(int sleepInterval){
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepInterval)); //to wait for a while before reproducing
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

    //attaching the producer to the buffer
    int size = sizeof(commodity)*bufferSize;
    buffer *b = attach_buffer(size); 
    sem_t *e = b->e;
    sem_t *mutex = b->mutex;
    sem_t *n = b->n;
    commodity *inbuff = b->inBuff;
    //int size = b->size;

 while (true) {

        commodity c = produce(commodity_name,mean,stddev);
        logMessage(commodity_name);
        sem_wait(e);  
        message = "Trying to get mutex on shared buffer\n";
        logMessage(commodity_name);    
        sem_wait(mutex);  
        add_to_buffer(c);
        logMessage(commodity_name);   
        sem_post(mutex);
        sem_post(n); 
        logMessage(commodity_name);
        message = "Sleeping for "+std::to_string(sleepInterval) + "\n";
        sleep(sleepInterval);
    }

    ///shmdt(mybuffer);
    sem_close(e);
    sem_close(n);
    sem_close(mutex);

 return 0;
}

