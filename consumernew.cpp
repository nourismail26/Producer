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
#include <sys/sem.h>
#include <cmath>
#include <fcntl.h>
#include <algorithm> //to sort alphabitcally
#include <unordered_map>
#define SHARED_MEM_KEY 2003
#define ERROR -1

struct commodity  {
    std::string name;
    double price;
};
struct TrackedCommodity {
    std::string name;
    double price;
    double avgPrice;
};
 
struct sembuf waiting= {0, -1, SEM_UNDO}; //decrementing the semaphore
struct sembuf signaling = {0, 1, SEM_UNDO};  //incrementin the semaphore
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

void consume(commodity c)
{
    std::cout << "Consuming commodity: " << c.name << std::endl;
    updateConsumed(c.name, c.price);
    print_table();
}

commodity take_from_buffer(commodity* &buff, int outBuff,int bufferSize)
{   
    if (buff == nullptr) {
        std::cerr << "Error: outBuff is nullptr." << std::endl;
        return {};  // Return empty commodity if error
    }
    commodity c = buff[outBuff];
    outBuff = (outBuff+1)%bufferSize;
    return c;
}

int semaphore_initialization(int key, int count){
    key_t k = ftok("for_the_key", key); 
    int semid = semget(k, 1, 0666 | IPC_CREAT); //returns the semaphore if present or create semaphore
    semctl(semid, 0, SETVAL, count); //initialize semaphore to the count
    return semid;
}

int get_shared_buffer(int size){ //returns the buffer id
    key_t key = SHARED_MEM_KEY;
    int buffer_id = shmget(key,size,0666 | IPC_CREAT); 
        //key -> unique & identified by the user 
        //buffer_id -> unique, returned by shmget & assigned by os  
    return buffer_id;
}

commodity* attach_to_buffer(int size){    //attach the producer to buffer
    int shared_buffer_id = get_shared_buffer(size);
    commodity *buffer;

    if (shared_buffer_id == ERROR){
        return NULL;
    }
    buffer = (commodity*) shmat(shared_buffer_id, NULL,0 );
    if (buffer == (commodity*)ERROR){
        return NULL;
    }
    return buffer;
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

    // initializing the semaphores
    int e = semaphore_initialization(10,bufferSize); //e->10
    int mutex = semaphore_initialization(20,1);  //mutex->20
    int n = semaphore_initialization(30,0);  //n->30
    
    //initialize buffer and an index -> for the first full place
    commodity *buffer = NULL; 
    int outBuff = 0;

    //attach to shared memory
    int buffSizeInBytes = bufferSize * sizeof(commodity);
    buffer = attach_to_buffer(buffSizeInBytes); //pointer to the first place in shared memory

    while (true) {
        std::cout << "\nWaiting on n"<< std::endl;    
        semop(n,&waiting,1); 
        std::cout << "\nWaiting on mutex"<< std::endl;     
        semop(mutex,&waiting,1);  
        commodity c  = take_from_buffer(buffer,outBuff,bufferSize);
        semop(mutex , &signaling,1);
        semop(e,&signaling,1);
        consume(c); 
    }

 return 0;
}

