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
#define ERROR -1
struct TrackedCommodity {
    std::string name;
    double price;
    double avgPrice;
};
TrackedCommodity consumed[5];
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
    if (b->outIndex < 0 || b->outIndex >= b->size) {
        std::cerr << "Error: outIndex is out of bounds." << std::endl;
        
        return {};
    }
    
    commodity c = b->commodities[b->outIndex];
    std::cout << "Commodity taken from buffer: Name = " << c.name
              << ", Price = " << c.price << std::endl;
    b->outIndex = (b->outIndex + 1) % b->size;
    
    return c;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: <program_name> <size of buffer>\n";
        return 1;
    }
    int bufferSize = std::stoi(argv[1]);
    if (bufferSize <= 0) {
        std::cerr << "Buffer size must be a positive integer.\n";
        return 1;
    }
    
    buffer *b = attach_buffer( ); // Attach to the shared memory buffer
    sem_t* e = &b->e;
    sem_t* mutex = &b->mutex;
    sem_t* n = &b->n;

    // Consumer loop
    while (true) {
        std::cout << "\nWaiting on n..." << std::endl;
        sem_wait(n);
        std::cout << "Waiting on mutex..." << std::endl;
        sem_wait(mutex);
        std::cout << "Consumer acquired mutex, modifying buffer..." << std::endl;
         printf("Consumer: size=%d, inIndex=%d, outIndex=%d\n", b->size, b->inIndex, b->outIndex);
         if (b->outIndex < 0 || b->outIndex >= b->size) {
    std::cerr << "Error: outIndex out of bounds.\n";
    sem_post(&b->mutex); // Release mutex if already acquired
    continue; // Skip to the next iteration
}

         // Take a commodity from the buffer
        commodity c = take_from_buffer(b);
        sem_post(mutex);
        std::cout << "Consumer released mutex." << std::endl;
        sem_post(e);
        consume(c);
    }

    shmdt(b);
    sem_close(e);
    sem_close(n);
    sem_close(mutex);

    return 0;
}
