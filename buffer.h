#ifndef BUFFER_H
#define BUFFER_H
#include <semaphore.h>

#define BUFFERSIZE 40
#define shm_key 1234

#include <sys/mman.h>
struct commodity  {
    char name[10];
    float price;
    int id ;//unique id for each commodity
};
struct buffer {
    commodity commodities[BUFFERSIZE];   // array of commodities 
    int head;                
    int tail;                
    int count;               
    sem_t mutex;             //semaphore for mutual exclusion
    sem_t empty;             //semaphore for empty slots
    sem_t full;              //semaphore for full slots
};

buffer* attach_buffer_to_shm(const char* shm_name) {
    void *ptr;
    const char* name = shm_name;
    
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        return nullptr;
    }

    // truncate shm to size of buffer structure
    if (ftruncate(fd, sizeof(buffer)) == -1) {  
        perror("ftruncate failed");
        close(fd);
        return nullptr;
    }

   //get ptr on shm
    ptr = mmap(nullptr, sizeof(buffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return nullptr;
    }

    close(fd);

    //return ptr to buffer attached to shm
    return static_cast<buffer*>(ptr);
}

void producer(buffer* shared_buffer);
void consumer(buffer* shared_buffer);

#endif