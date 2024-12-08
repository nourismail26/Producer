#include "buffer.h"


#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <semaphore.h>
#include <sys/shm.h>
#include <unistd.h>
#include "buffer.h" 

#define shm_key 1234
struct TrackedCommodity {
    std::string name;
    double price;
    double avgPrice;
};
TrackedCommodity consumed[5];
int N_input;
int count = 0; // Number of currently tracked commodities

// Map to store price history for calculating averages
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
    // Update price history for the commodity
    priceHistory[name].push_back(price);
    if (priceHistory[name].size() > 4) {
        priceHistory[name].erase(priceHistory[name].begin());
    }
    if (count < 5) {
        consumed[count++] = {name, price, calculateAverage(name)};
    } else {
        // Shift older commodities to make space for the new one
        for (int i = 0; i < 4; ++i) {
            consumed[i] = consumed[i + 1];
        }
        consumed[4] = {name, price, calculateAverage(name)};
    }
    std::sort(consumed, consumed + count, [](const TrackedCommodity& a, const TrackedCommodity& b) {
        return a.name < b.name;
    });
}
void print_table() {
    printf("+-------------------------------------------+\n");
    printf("| Commodity  |   Price   |   AvgPrice       |\n");
    printf("+-------------------------------------------+\n");

    for (int i = 0; i < count; ++i) {
        printf("| %-10s | \033[;34m%7.2lf\033[0m | \033[;34m%10.2lf\033[0m |\n",
               consumed[i].name.c_str(), consumed[i].price, consumed[i].avgPrice);
    }
    for (int i = count; i < 5; ++i) {
        printf("| %-10s | \033[;34m%7.2lf\033[0m | \033[;34m%10.2lf\033[0m |\n", "", 0.00, 0.00);
    }

    printf("+-------------------------------------------+\n");
}
void consume(commodity c) {
    std::cout << "Consuming commodity: " << c.name << " with price: " << c.price << std::endl;
    updateConsumed(c.name, c.price);
    print_table();
}
commodity take_from_buffer(buffer* b) {
    if (b->out_index < 0 || b->out_index >= N_input) {
        std::cerr << "Error: out_index is out of bounds." << std::endl;
        return {};
    }
    
    commodity c = b->items[b->out_index];
    std::cout << "Commodity taken from buffer: Name = " << c.name
              << ", Price = " << c.price << std::endl;
    b->out_index = (b->out_index + 1) % N_input;
    
    return c;
}
void consumer(int argc, char* argv[]) {
    // Attach to shared memory
    if (argc < 2) {
        std::cerr << "Usage: <program_name> <size of buffer>\n";
        return ;
    }
    N_input = std::stoi(argv[1]);
    if (N_input <= 0) {
        std::cerr << "Buffer size must be a positive integer.\n";
        return ;
    }
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

    // Consume an item
    sem_wait(full);   // Wait for a full slot
    sem_wait(mutex);  // Lock the critical section
     commodity c = take_from_buffer(buf);
    commodity item = buf->items[buf->out_index];
    buf->out_index = (buf->out_index + 1) % BUFFER_SIZE;

    std::cout << "Consumed: " << item.name << ", " << item.price << "\n";

    sem_post(mutex);  // Unlock the critical section
    sem_post(empty);  // Signal an empty slot
     consume(c);
}

int main(int argc, char* argv[]) {
    while(1)
    consumer(argc,argv);
    return 0;
}