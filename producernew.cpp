#include "buffer.h"
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
int global_commodity_id = 0;  // Global counter to track IDs

double generate_price(double mean, double stddev){
    static std::default_random_engine generator;
    std::normal_distribution<double> dist(mean, stddev);
    return dist(generator);
}
commodity intialize_commodity(const char* Name , double mean , double stddev){
    commodity c;
    strncpy(c.name, Name, sizeof(c.name) - 1);
    c.name[sizeof(c.name) - 1] = '\0'; 
    c.price = generate_price(mean, stddev);

    // Get a unique ID from shared memory
    c.id = get_next_id();
    message = "Generating a new value of " + std::to_string(c.price) + " with ID: " + std::to_string(c.id) + "\n";
    return c;

}
commodity produce(const char* Name, double mean, double stddev, int id) {
    commodity c;
    strncpy(c.name, Name, sizeof(c.name) - 1);
    c.name[sizeof(c.name) - 1] = '\0'; 
    c.price = generate_price(mean, stddev);

    // Use the provided ID to ensure the same ID is used for each commodity
    c.id = id;
    message = "Generating a new value of " + std::to_string(c.price) + " with ID: " + std::to_string(c.id) + "\n";
    return c;
}
bool add_to_buffer(buffer* b, commodity c,int N_input) {
    // Add the commodity to the buffer at the current in_index
    b->items[b->in_index] = c;

    // Increment the in_index and wrap around for circular buffer
    b->in_index = (b->in_index + 1) % N_input;

    // Log the action
    message = "Added a commodity to buffer: " + std::string(c.name)+ " with price " + std::to_string(c.price) + "\n";
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
void producer(const char* name, double mean,double stddev, int sleepInterval ,int N_input) {
    // Attach to shared memory
    buffer* buf = attach_to_buffer();
    if (!buf) {
        std::cerr << "Error: Failed to attach to shared buffer.\n";
        exit(1);
    }

    // Open semaphores
    sem_t* empty = sem_open(SEM_EMPTY, 0);
    sem_t* full = sem_open(SEM_FULL, 0);
    sem_t* mutex = sem_open(SEM_MUTEX, 0);

    if (!empty || !full || !mutex) {
        std::cerr << "Error: Failed to open semaphores.\n";
        exit(1);
    }
    static commodity item = intialize_commodity(name, mean, stddev);
    // Produce an item
    sem_wait(empty);  // Wait for an empty slot
   commodity produced_item = produce(name, mean, stddev, item.id);
    logMessage(name);
    message = "Trying to get mutex on shared buffer\n";
    logMessage(name);
    sem_wait(mutex);  // Lock the critical section
    std::cout << "Producer acquired mutex, modifying buffer..." << std::endl;
        if (add_to_buffer(buf, item,N_input)) {
            logMessage(name);  // Log success
        }
   // commodity item;
   // strncpy(item.name, name, sizeof(item.name) - 1);
   // item.price = price;

    std::cout << "Produced: " << item.name << ", " << item.price << "\n";

    sem_post(mutex);  // Unlock the critical section
    sem_post(full);   // Signal a full slot
     message = "Sleeping for " + std::to_string(sleepInterval) + " ms\n";
        logMessage(name);
        sleep(sleepInterval);
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "init") {
        initialize_shared_resources();
        std::cout << "Shared resources initialized.\n";
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "clean") {
        cleanup_shared_resources();
        std::cout << "Shared resources cleaned.\n";
        return 0;
    }
     if (argc < 6) {
        std::cerr << "Usage: <program_name> <commodity> <mean> <stddev> <sleep interval> <size of buffer>\n";
        return 1;
    }

    const char* name = argv[1];
    double mean = std::stod(argv[2]);
    double stddev = std::stod(argv[3]);
    int sleepInterval = std::stoi(argv[4]); // in milliseconds
    int N_input = std::stoi(argv[5]);

    if (N_input <= 0) {
        std::cerr << "Buffer size must be a positive integer.\n";
        return 1;
    }

    if (strlen(name) >= 10) {
        std::cerr << "Error: Commodity name must be shorter than 10 characters.\n";
        return 1;
    }
  /*  if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <name> <price>\n";
        return 1;
    }

    const char* name = argv[1];
    double price = std::stod(argv[2]);*/
while(1)
    producer(name,mean,stddev,sleepInterval,N_input);
    return 0;
}
