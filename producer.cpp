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
bool add_to_buffer(buffer* b, commodity c) {
    // Add the commodity to the buffer at the current inIndex
    b->commodities[b->inIndex] = c;

    // Increment the inIndex and wrap around for circular buffer
    b->inIndex = (b->inIndex + 1) % b->size;

    // Log the action
    message = "Added a commodity to buffer: " + c.name + " with price " + std::to_string(c.price) + "\n";
    return true;
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
    // Validate arguments
    if (argc < 6) {
        std::cerr << "Usage: <program_name> <commodity> <mean> <stddev> <sleep interval> <size of buffer>\n";
        return 1;
    }

    std::string commodity_name = argv[1];
    double mean = std::stod(argv[2]);
    double stddev = std::stod(argv[3]);
    int sleepInterval = std::stoi(argv[4]); // in milliseconds
    int bufferSize = std::stoi(argv[5]);

    if (bufferSize <= 0) {
        std::cerr << "Buffer size must be a positive integer.\n";
        return 1;
    }

    if (commodity_name.length() >= 10) {
        std::cerr << "Error: Commodity name must be shorter than 10 characters.\n";
        return 1;
    }
 // Initialize buffer structure and semaphores (only done by the producer)
   buffer* b = initialize_buffer(bufferSize);
    // Producer loop
    while (true) {
        std::cout << "\nWaiting on e..." << std::endl;
        sem_wait(&b->e);  // Wait until there’s space in the buffer
        commodity c = produce(commodity_name, mean, stddev);
        logMessage(commodity_name);
        message = "Trying to get mutex on shared buffer\n";
        logMessage(commodity_name);
        std::cout << "Waiting on mutex..." << std::endl;
        sem_wait(&b->mutex);  // Enter critical section
        std::cout << "Producer acquired mutex, modifying buffer..." << std::endl;
        if (add_to_buffer(b, c)) {
            logMessage(commodity_name);  // Log success
        }
        sem_post(&b->mutex);  // Leave critical section
        sem_post(&b->n);      // Signal that there’s a new full slot in the buffer
        message = "Sleeping for " + std::to_string(sleepInterval) + " ms\n";
        logMessage(commodity_name);
        sleep(sleepInterval);  // Sleep for the specified interval
    }

    // Cleanup (done by the producer when exiting)
    shmdt(b);
    sem_destroy(&b->e);
    sem_destroy(&b->mutex);
    sem_destroy(&b->n);
    
    return 0;
}


