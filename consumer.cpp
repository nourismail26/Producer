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

double consumed[11][4]; //matrix -> to track el last 4 consumed
double table[11][2] = {0.0}; //matrix -> to keep the prints of the table
const char *commodities[11] = {
        "ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD",
        "LEAD", "MENTHAOIL", "NATURALGAS", "NICKEL", "SILVER", "ZINC"
    };


double calculateAverage(int row,double price) {
    double sum = price; //ptice el currrent
    int count = 1;
  for (int i=0;i<4;i++) { //loop tegeeb prices ely ably
        if (consumed[row][i] > 0.0) {
            sum += consumed[row][i];
            count++;
        }
    }

    return (count > 0) ? sum / count : 0.0;
}

void update_commodity_prices(double new_price, int row) {
   for (int i = 3; i > 0; --i) {
        consumed[row][i] = consumed[row][i - 1];
    }
    consumed[row][0] = new_price; 
}
int getRow( char* name){
    // badawar 3ala el asm 3ashan arga3 rakamo
    for (int i = 0; i < 11; i++) {
        if (strcmp(name, commodities[i]) == 0) {
            return i;  // la2eeto
        }
    }

    return -1;  // mal2ethoosh
}
void printCommodityTable() {
    // Print the table header
    printf("+-------------------------------------------+\n");
    printf("| Commodity     |   Price   |   AvgPrice    |\n");
    printf("+-------------------------------------------+\n");

    const char *blue = "\033[34m";   // Blue color
    const char *reset = "\033[0m";   // Reset color to default
    const char *red = "\033[;31m";     // Red for decrease
    const char *green = "\033[;32m";     

    // Loop through each commodity and print the details
    for (int i = 0; i < 11; i++) {
        printf("| %-12s | ", commodities[i]);
        
        if (table[i][0] == 0.0) {
            printf("%s%9.3f%s", blue, table[i][0], reset);
        } else {
                if (consumed[i][1] < consumed[i][0]) // if i increased 
                printf("%s%9.3f%s", green, table[i][0], reset);
                else
                 printf("%s%9.3f%s", red, table[i][0], reset);
        }

        printf(" | ");
        
        if (table[i][1] == 0.0) { //print el default -> blue
            printf("%s%12.3f%s", blue, table[i][1], reset);
        } else {
             if (consumed[i][1] < consumed[i][0]) // if i increased -> green
            printf("%s%12.3f%s↑",green, table[i][1], reset);
            else                                  //if i decresed -> red
            printf("%s%12.3f%s↓",red, table[i][1], reset);
        }
            
        printf(" |\n");
    }

    printf("+-------------------------------------------+\n");
}

void consume(commodity c,buffer *b) {
    double price = c.price;
    int row = getRow(c.name);
    table[row][0] = price;
    double average = calculateAverage(row,price);
    table[row][1] = average;
    update_commodity_prices(price,row);
    printCommodityTable(); 
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

void sleep(int sleepInterval){
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepInterval)); 
}

void consumer() {

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
        sem_wait(full);   
        sem_wait(mutex);  
        commodity c = take_from_buffer(buf);
        sem_post(mutex);  
        sem_post(empty);  
        consume(c,buf);
        sleep(200); 
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
    
    consumer();

    return 0;
}
