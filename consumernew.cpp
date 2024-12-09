#include "buffer.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <semaphore.h>
#include <sys/shm.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>

#define shm_key 1234

int count = 0; // Number of currently tracked commodities
double calculateAverage(const commodity& c) {
    double sum = 0.0;
    int count = 0;
  for (double price : c.priceHistory) {
        if (price > 0.0) {
            sum += price;
            count++;
        }
    }

    return (count > 0) ? sum / count : 0.0;
}
void update_commodity_prices(commodity& c,double new_price) {
   for (int i = 3; i > 0; --i) {
        c.priceHistory[i] = c.priceHistory[i - 1];
    }
    c.priceHistory[0] = new_price; // Set the new price as the first element
}
void print_table(buffer* buf, int buffer_size) {
    const char* commodityNames[11] = {
        "ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD", 
        "LEAD", "MENTHAOIL", "NATURALGAS", "NICKEL", "SILVER", "ZINC"
    };

    printf("+-------------------------------------------+\n");
    printf("| Commodity     |   Price   |   AvgPrice    |\n");
    printf("+-------------------------------------------+\n");

    // Loop through all known commodity names
    for (const auto& name : commodityNames) {
        double price = 0.00;     // Default price is 0.00
        double avgPrice = 0.00;  // Default average price is 0.00
        const char* arrow = " "; // Default arrow for no change
        const char* color = "\033[;34m"; // Default color (blue)
         // Find the matching commodity in the buffer
        for (int i = 0; i < buffer_size; ++i) {
            commodity& c = buf->items[i];
            
            // Check if the current commodity matches the name
            if (strcmp(c.name, name) == 0) {
                // Calculate average price
                avgPrice = calculateAverage(c);
                
                // Determine price trend
                if (c.priceHistory[1] != 0.00) { // If there are previous prices
                     if (c.price > c.priceHistory[1]) {
                        arrow = "\u2191";        // Unicode arrow up
                        color = "\033[;32m";     // Green for increase
                    } else if (c.price < c.priceHistory[1]) {
                        arrow = "\u2193";        // Unicode arrow down
                        color = "\033[;31m";     // Red for decrease
                    }
                }

                // Store the current price in the price history  //TODO: when should we update commodity exactly
                update_commodity_prices(c, c.price);
                price = c.price; // Current price
                break; // Exit loop once commodity is found
            }
        }
       printf("| %-12s | %s%8.3lf\033[0m | %s%10.3lf\033[0m  %s |\n", 
               name, color, price,color, avgPrice, arrow);
    }

    printf("+-------------------------------------------+\n");
    fflush(stdout);
}



void consume(commodity c,buffer *b) {
   system("clear"); //To print one table
     print_table(b,b->size); 
}

commodity take_from_buffer(buffer* b) {
    if (b->out_index < 0 || b->out_index >= b->size) {
        std::cerr << "Error: out_index is out of bounds." << std::endl;
        return {};
    }

    commodity c = b->items[b->out_index];
     b->out_index = (b->out_index + 1) % b->size;

    return c;
}

void consumer(int argc, char* argv[]) {

    int buffersize =  atoi(argv[1]);
    initialize_shared_resources(buffersize);
    buffer* buf = attach_to_buffer();
    //int buffer_size = buf->size;
    if (!buf) {
        std::cerr << "Error: Failed to attach to shared buffer.\n";
        exit(1);
    }

    sem_t* empty = sem_open(SEM_EMPTY, 0);
    sem_t* full = sem_open(SEM_FULL, 0);
    sem_t* mutex = sem_open(SEM_MUTEX, 0);

    if (!empty || !full || !mutex) {
        std::cerr << "Error: Failed to open semaphores.\n";
        exit(1);
    }

    while (true) {
        sem_wait(full);   
        sem_wait(mutex);  
        commodity c = take_from_buffer(buf);
        sem_post(mutex);  
        sem_post(empty);  
        consume(c,buf);
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); 
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: <program_name> <size of buffer>\n";
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "clean") {
        cleanup_shared_resources();
        std::cout << "Shared resources cleaned.\n";
        return 0;
    }
    int buffer_size = std::stoi(argv[1]);
if (buffer_size <= 0) {
    std::cerr << "Buffer size must be a positive integer.\n";
    exit(1);
}
     initialize_shared_resources( buffer_size );
        std::cout << "Shared resources initialized.\n";
        
    consumer(argc, argv);
    return 0;
}
