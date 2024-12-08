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

    // Update price history
    auto& prices = priceHistory[key];
    prices.push_back(c.price);//adds element to end of prices vector
    if (prices.size() > 4) {  // Keep up to 4 historical prices
        prices.erase(prices.begin());
    }
    for (const auto& pair : priceHistory) {
    printf("Commodity: %s, ID: %d, History: ", pair.first.name.c_str(), pair.first.id);
    for (double p : pair.second) {
        printf("%.2f ", p);
    }
    printf("\n");
}


    // Update the consumed array
    double avgPrice = calculateAverage(key);
    if (count < 5) {
        consumed[count++] = {c, avgPrice};
    } else {
        for (int i = 0; i < 4; ++i) {
            consumed[i] = consumed[i + 1];
        }
        consumed[4] = {c, avgPrice};
    }

    // Sort by name for display
    std::sort(consumed, consumed + count, [](const TrackedCommodity& a, const TrackedCommodity& b) {
        return a.c.name< b.c.name;
    });

   // printf("Contents of consumed array before searching:\n");
    //for (int i = 0; i < count; ++i) {
    //printf("Consumed[%d]: %s, Price: %.2f, AvgPrice: %.2f\n", i, consumed[i].c.name, consumed[i].c.price, consumed[i].avgPrice);
   //}
}

void print_table() {
   const char* commodityNames[11] = {
        "ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD", 
        "LEAD", "MENTHAOIL", "NATURALGAS", "NICKEL", "SILVER", "ZINC"
    };

    printf("+-------------------------------------------+\n");
    printf("| Commodity    |   Price   |   AvgPrice    |\n");
    printf("+-------------------------------------------+\n");

   for (const auto& name : commodityNames) {
        double price = 0.00;
        double avgPrice = 0.00;
        const char* arrow = " ";             
        const char* color = "\033[;34m";     // Default: blue for no change

        // Find the commodity in the consumed array
        auto it = std::find_if(consumed, consumed + count, [&name](const TrackedCommodity& tc) {
             return tc.c.name == std::string(name); 
        });

        if (it != consumed + count) {//exits in consumed array (array of trackedcommoditries)
            price = it->c.price;
            
            // Create the CommodityKey for the current commodity
            CommodityKey key = {name, it->c.id};  

            // Retrieve the price history for the commodity using the key
            auto& history = priceHistory[key];

            // If there is any previous price, compare it with the current price
            if (!history.empty()) {
                double lastPrice = history.back(); // The most recent price
    
                if (price > lastPrice) {
                    arrow = "\u2191";        // Unicode arrow up
                    color = "\033[;32m";     // Green for increase
                } else if (price < lastPrice) {
                    arrow = "\u2193";        // Unicode arrow down
                    color = "\033[;31m";     // Red for decrease
                }
            }

            // Add the current price to the price history
            history.push_back(price);

            // Calculate the average price from the price history
            double sum = std::accumulate(history.begin(), history.end(), 0.0);
            avgPrice = sum / history.size();
        }

        // Print the table row for the commodity
        printf("| %-12s | %s%8.2lf\033[0m | %10.2lf %s |\n", 
               name, color, price, avgPrice, arrow);
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

        commodity c = take_from_buffer(buf);

        sem_post(mutex);  // Unlock the critical section
        sem_post(empty);  // Signal an empty slot

        consume(c);
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Simulate delay
    }
}

int main(int argc, char* argv[]) {
    consumer(argc, argv);
    return 0;
}
