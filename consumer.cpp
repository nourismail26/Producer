#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <cstring>
#include <condition_variable>
#include <random>
#include <fcntl.h>

#define shm_key 1234
#define SHARED_MEM_KEY 1234
#define SHARED_ID_KEY 5678
#define ERROR -1
// Semaphore names
const char* SEM_EMPTY = "/sem_empty";
const char* SEM_FULL = "/sem_full";
const char* SEM_MUTEX = "/sem_mutex";
const char* SEM_ID_MUTEX = "/sem_id_mutex";
const char* SEM_MAX_PROD = "/sem_max_prod";
double consumed[11][4]; //matrix -> to track el last 4 consumed
double table[11][2] = {0.0}; //matrix -> to keep the prints of the table
const char *commodities[11] = {
        "ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD",
        "LEAD", "MENTHAOIL", "NATURALGAS", "NICKEL", "SILVER", "ZINC"
    };
struct commodity {
    char name[32];
    double price;
    int id;
    double priceHistory[4];
};

//shared buff struct
struct buffer {
    int in_index;                 //awel heta fadia
    int out_index;                  //awel heta malyana
    int size ;                      //mehtagah 3ashan el consume fn
    commodity items[];          
};
//initialize -> 1.shared mem    2.semaphores
void initialize_shared_resources(int buffer_size) {
     printf("Requested shared memory size: %zu bytes\n", sizeof(buffer) + (buffer_size * sizeof(commodity) ));
     int shm_id = shmget(SHARED_MEM_KEY, sizeof(buffer) + buffer_size * sizeof(commodity), IPC_CREAT | 0666);
     if (shm_id == -1) {
       perror("shmget failed here inn");
       exit(1);
          }  
    buffer* buf = (buffer*)shmat(shm_id, nullptr, 0);
    if (buf == (void*)ERROR) {
        perror("shmat failed");
        exit(1);
    }
      buf->size = buffer_size;
      buf->in_index=0;
      buf->out_index=0;
       
       shmdt(buf);
    int id_shm_id = shmget(SHARED_ID_KEY, sizeof(int), IPC_CREAT | 0666);
    if (id_shm_id == ERROR) {
        perror("shmget (ID counter) failed");
        exit(1);
    }
    int* id_counter = (int*)shmat(id_shm_id, nullptr, 0);
    if (id_counter == (void*)ERROR) {
        perror("shmat (ID counter) failed");
        exit(1);
    }
    *id_counter = 1; //start IDs from 1
    shmdt(id_counter);

    //initialize semaphores
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_ID_MUTEX);
    sem_unlink(SEM_MAX_PROD);

    sem_open(SEM_EMPTY, O_CREAT, 0644, buffer_size);  // Initially buffer_size empty slots
    sem_open(SEM_FULL, O_CREAT, 0644, 0);            // Initially no full slots
    sem_open(SEM_MUTEX, O_CREAT, 0644, 1);           // Mutex starts unlocked
    sem_open(SEM_ID_MUTEX, O_CREAT, 0644, 1);        // Mutex for ID counter
    sem_open(SEM_MAX_PROD, O_CREAT, 0644, 20);    //CHANGE TO 20
}
//attach to shared mem
buffer* attach_to_buffer() {
    int shm_id = shmget(SHARED_MEM_KEY, 0, 0666);
    if (shm_id == ERROR) {
        perror("shmget failed ");
        return nullptr;
    }

    buffer* buf = (buffer*)shmat(shm_id, nullptr, 0);
    if (buf == (void*)ERROR) {
        perror("shmat failed");
        return nullptr;
    }

    return buf;
}
//get the next unique ID
int get_next_id() {
    int shm_id = shmget(SHARED_ID_KEY, sizeof(int), 0666);
    if (shm_id == ERROR) {
        perror("shmget (ID counter) failed");
        exit(1);
    }

    int* id_counter = (int*)shmat(shm_id, nullptr, 0);
    if (id_counter == (void*)ERROR) {
        perror("shmat (ID counter) failed");
        exit(1);
    }

    //increment the ID atomically
    sem_t* id_mutex = sem_open(SEM_ID_MUTEX, 0);
    sem_wait(id_mutex);
    int id = (*id_counter)++;
    sem_post(id_mutex);
    sem_close(id_mutex);

    shmdt(id_counter);
    return id;
}
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
            printf("%s%12.3f↑%s",green, table[i][1], reset);
            else                                  //if i decresed -> red
            printf("%s%12.3f↓%s",red, table[i][1], reset);
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

// Clean up shared resources
void cleanup_shared_resources() {
    shmctl(shmget(SHARED_MEM_KEY, sizeof(buffer), 0666), IPC_RMID, nullptr);
    shmctl(shmget(SHARED_ID_KEY, sizeof(int), 0666), IPC_RMID, nullptr);

    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_ID_MUTEX);
    sem_unlink(SEM_MAX_PROD);
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
