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
#include <algorithm> //to sort alphabitcally
#include <unordered_map>
#include "buffer.h"
#define shm_key 1234
#define ERROR -1
struct TrackedCommodity {
    std::string name;
    double price;
    double avgPrice;
};
 
    TrackedCommodity consumed[5]; // array to track the last five commodities
    int count = 0;//number of currently tracked commodities
   // Map to store price history for each commodity
std::unordered_map<std::string, std::vector<double>> priceHistory;

double calculateAverage(const std::string& name) {
    const auto& prices = priceHistory[name];
    double sum = 0.0;
    for (double price : prices) {
        sum += price;
    }
    return prices.empty() ? 0.0 : sum / prices.size();
}
void updateConsumed(const std::string& name, double price) {
    // Update the price history
    priceHistory[name].push_back(price);
 if (priceHistory[name].size() > 4) {
        priceHistory[name].erase(priceHistory[name].begin());
    }
 if (count < 5) {
        consumed[count++] = {name, price, calculateAverage(name)};
    } else {
       //shift older commodities
        for (int i = 0; i < 4; ++i) {
            consumed[i] = consumed[i + 1];
        }
        consumed[4] = {name, price, calculateAverage(name)};
    }

    //sort alpha
    std::sort(consumed, consumed + count, [](const TrackedCommodity& a, const TrackedCommodity& b) {
        return a.name < b.name;
    });
}

void print_table() {
   
     printf("+------------------------------------------------+\n");
    printf("| Commodity  |   Price   |   AvgPrice           |\n");
    printf("+------------------------------------------------+\n");


   
    for (int i = 0; i < count; ++i) {
        printf("| %-10s | \033[;34m%7.2lf\033[0m | \033[;34m%10.2lf\033[0m |\n",
               consumed[i].name.c_str(), consumed[i].price, consumed[i].avgPrice);
    }
    for (int i = count; i < 5; ++i) {
        printf("| %-10s | \033[;34m%7.2lf\033[0m | \033[;34m%10.2lf\033[0m |\n", "", 0.00, 0.00);
    }
    printf("+--------------------------------------------------+\n");
}

void consume(commodity c,TrackedCommodity *consumed)
{
     std::cout << "Consuming commodity: " << c.name << std::endl;
     updateConsumed(c.name, c.price);
    print_table();
}

commodity take_from_buffer(commodity* &outBuff, buffer* b)
{   
    if (outBuff == nullptr) {
        std::cerr << "Error: outBuff is nullptr." << std::endl;
        return {};  // Return empty commodity if error
    }
    commodity c = *outBuff;
    //outBuff = (outBuff + 1)%size;
    outBuff--;
     if (outBuff < b->inBuff) {
          outBuff = b->inBuff + b->size - 1; //NOTSURE!!!
    }
    return c;
}


int main(int argc, char* argv[]){
    
    //checking for the number of arguments
    if (argc < 1) {
        std::cerr << "Usage: <program_name>  <size of buffer>\n";
        return 1;
    }

     int bufferSize = std::stoi(argv[1]);
    if (bufferSize <= 0) {
        std::cerr << "Buffer size must be a positive integer.\n";
        return 1;
        }

    //attaching the consumer to the buffer
    int size = sizeof(commodity)*bufferSize; 
    buffer *b = attach_buffer(size);  //fi attach buffer by ignore el size law el shared memory created asln
    sem_t *e = b->e;
    sem_t *mutex = b->mutex;   
    sem_t *n = b->n;
    commodity *outBuff = b->outBuff;

    while (true) {
        std::cout << "\nWaiting on n"<< std::endl;    
        sem_wait(n); 
        std::cout << "\nWaiting on mutex"<< std::endl;     
        sem_wait(mutex);  
        commodity c  = take_from_buffer(outBuff,b);
        sem_post(mutex);
        sem_post(e);
        consume(c,consumed); 
    }

    ///shmdt(mybuffer);
    sem_close(e);
    sem_close(n);
    sem_close(mutex);

 return 0;
}

