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
   
    c.id = get_next_id();
    std::fill(std::begin(c.priceHistory), std::end(c.priceHistory), 0.00);
    message = "Generating a new value of " + std::to_string(c.price) + " with ID: " + std::to_string(c.id) + "\n";
    printf("Commodity initialized: Name = %s, ID = %d, Initial Price = %.2f\n", c.name, c.id, c.price);
    return c;

}
void update_commodity_prices(commodity& c) {
   for (int i = 3; i > 0; --i) {
        c.priceHistory[i] = c.priceHistory[i - 1];
    }
    c.priceHistory[0] = c.price; // Set the new price as the first element
}

commodity produce(commodity item,int mean,int stddev) {
    
    item.price = generate_price(mean, stddev);
    update_commodity_prices(item); 
    message = "Generating a new value of " + std::to_string(item.price) + " with ID: " + std::to_string(item.id) + "\n";
    return item;
}

bool add_to_buffer(buffer* b, commodity c,int N_input) {
    b->items[b->in_index] = c;
    b->in_index = (b->in_index + 1) % N_input;
    message = "Added a commodity to buffer: " + std::string(c.name)+ " with price " + std::to_string(c.price) + "\n";
    printf("Added to buffer with  Price History: ");
for (double price : c.priceHistory) {
    printf("%.2f ", price);
}
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
    while(true){
    sem_wait(empty);  // Wait for an empty slot
    item= produce( item,mean,stddev);          
    logMessage(name);
    message = "Trying to get mutex on shared buffer\n";
    logMessage(name);
    sem_wait(mutex);  
    std::cout << "Producer acquired mutex, modifying buffer..." << std::endl;
        if (add_to_buffer(buf,item,N_input)) {
            logMessage(name);  // Log success
        }
    std::cout << "Produced: " << item.name << ", " << item.price << "\n";
    sem_post(mutex);  
    sem_post(full);   
    message = "Sleeping for " + std::to_string(sleepInterval) + " ms\n";
    logMessage(name);
    sleep(sleepInterval);
        }
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
//while(1)
    producer(name,mean,stddev,sleepInterval,N_input);
    return 0;
}
