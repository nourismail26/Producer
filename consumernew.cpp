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

// Data structures for tracking commodities
struct CommodityKey {
    std::string name;
    int id;

    bool operator==(const CommodityKey& other) const {
        return name == other.name && id == other.id;
    }
};

// Custom hash function for CommodityKey
struct CommodityKeyHash {
    std::size_t operator()(const CommodityKey& key) const {
        return std::hash<std::string>()(key.name) ^ std::hash<int>()(key.id);
    }
};

std::unordered_map<CommodityKey, std::vector<double>, CommodityKeyHash> priceHistory;
struct TrackedCommodity {
    commodity c;
    double avgPrice;
};
TrackedCommodity consumed[5];
int N_input;
int count = 0; // Number of currently tracked commodities
double calculateAverage(const CommodityKey& key) {
    const auto& prices = priceHistory[key];// auto automatically set type and & for refrence to prices not the actual value
    if (prices.empty()) return 0.0;

    double sum = 0.0;
    for (double price : prices) {
        sum += price;
    }
    return sum / prices.size();
}

void updateConsumed(commodity c) {
    CommodityKey key = {c.name, c.id};
    // Update price history for the commodity
    auto& prices = priceHistory[key];
    prices.push_back(c.price);  // Add the current price
    if (prices.size() > 5) {    // Keep up to 5 prices (current + last 4)
        prices.erase(prices.begin());
    }
    // Debugging: Print the price history for verification
    printf("UPDATED PRICES :Commodity: %s, ID: %d, History: ", key.name.c_str(), key.id);
    for (double p : prices) {
        printf("%.2f ", p);
    }
    printf("\n");
    // Calculate the average of the last 5 prices
    double avgPrice = calculateAverage(key);
    // Check if the commodity already exists in the consumed array
    bool updated = false;
    for (int i = 0; i < count; ++i) {
        if (strcmp(consumed[i].c.name, c.name) == 0) {  // Match by name
            consumed[i] = {c, avgPrice};               // Update commodity details
            updated = true;
            break;
        }
    }
    // If not found and the array is not full, add the commodity
    if (!updated) {
        if (count < 4) {
            consumed[count++] = {c, avgPrice};
        } else {  // Shift the oldest out if the array is full
            for (int i = 0; i < 4; ++i) {
                consumed[i] = consumed[i + 1];
            }
            consumed[4] = {c, avgPrice};
        }
    }
    std::sort(consumed, consumed + count, [](const TrackedCommodity& a, const TrackedCommodity& b) {
        return strcmp(a.c.name, b.c.name) < 0;
    });
}

void print_table() {
    const char* commodityNames[11] = {
        "ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD", 
        "LEAD", "MENTHAOIL", "NATURALGAS", "NICKEL", "SILVER", "ZINC"
    };

    printf("+-------------------------------------------+\n");
    printf("| Currency     |   Price   |   AvgPrice    |\n");
    printf("+-------------------------------------------+\n");

    // Loop through all known commodity names
    for (const auto& name : commodityNames) {
        double price = 0.00;     // Default price is 0.00
        double avgPrice = 0.00;  // Default average price is 0.00
        const char* arrow = " "; // Default arrow for no change
        const char* color = "\033[;34m"; // Default color (blue)

        // Check if the commodity is in the price history
        bool found = false;
        for (const auto& pair : priceHistory) {
            if (std::string(pair.first.name) == name) {
                found = true;
                price = pair.second.back(); // Current price is the last entry
                avgPrice = std::accumulate(pair.second.begin(), pair.second.end(), 0.0) / pair.second.size();

                // Compare the current price to the previous one, if available
                if (pair.second.size() > 1) {
                    double lastPrice = pair.second[pair.second.size() - 2];
                    if (price > lastPrice) {
                        arrow = "\u2191";        // Unicode arrow up
                        color = "\033[;32m";     // Green for increase
                    } else if (price < lastPrice) {
                        arrow = "\u2193";        // Unicode arrow down
                        color = "\033[;31m";     // Red for decrease
                    }
                }
                break;
            }
        }

        // Print the row for the commodity
        printf("| %-12s | %s%8.2lf\033[0m | %10.2lf %s |\n", 
               name, color, price, avgPrice, arrow);

        // If the commodity is not found, it remains at default values
    }

    printf("+-------------------------------------------+\n");
    fflush(stdout);
}


void consume(commodity c) {
   // std::cout << "Consuming commodity: " << c.name << " with price: " << c.price << std::endl;
    updateConsumed(c);
    print_table();
}

commodity take_from_buffer(buffer* b) {
    if (b->out_index < 0 || b->out_index >= N_input) {
        std::cerr << "Error: out_index is out of bounds." << std::endl;
        return {};
    }

    commodity c = b->items[b->out_index];
    //std::cout << "Commodity taken from buffer: Name = " << c.name
      //        << ", Price = " << c.price << std::endl;
    b->out_index = (b->out_index + 1) % N_input;

    return c;
}

void consumer(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: <program_name> <size of buffer>\n";
        return;
    }

    N_input = std::stoi(argv[1]);
    if (N_input <= 0) {
        std::cerr << "Buffer size must be a positive integer.\n";
        return;
    }

    buffer* buf = attach_to_buffer();
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
        sem_wait(full);   // Wait for a full slot
        sem_wait(mutex);  // Lock the critical section

        //commodity c = take_from_buffer(buf);
         commodity c = buf->items[buf->out_index];
        buf->out_index = (buf->out_index ++) % BUFFER_SIZE; // Circular buffer behavior

        sem_post(mutex);  // Unlock the critical section
        sem_post(empty);  // Signal an empty slot
        
        consume(c);
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Simulate delay
    }
}

int main(int argc, char* argv[]) {
   // while(1)
    consumer(argc, argv);
    return 0;
}
