#ifndef BUFFER_H
#define BUFFER_H
#include <semaphore.h>
#include <string>
#include <cstddef>
#include <sys/shm.h>
#define shm_key 1234
#define ERROR -1
#include <fcntl.h>  



struct commodity  {
    std::string name;
    double price;
};
#pragma pack(1)  // Ensure no padding
struct buffer { 
    int size;               // Actual size of the buffer
    int inIndex;            // Points to the next empty slot
    int outIndex;           // Points to the next full slot
    sem_t e;               // Semaphore for empty slots
    sem_t n;               // Semaphore for full slots
    sem_t mutex;           // Mutex for critical sections
    commodity commodities[]; // Flexible array for commodities
};


int get_shared_buffer(int buffersize){ //returns the buffer id
    key_t key = shm_key;
    std::cout << "shmget: key = " << shm_key << ", size = " << buffersize << std::endl;

    int buffer_id = shmget(key,buffersize,0666 | IPC_CREAT); 
        //key -> unique & identified by the user 
        //buffer_id -> unique, returned by shmget & assigned by os  
    return buffer_id;
}
buffer* initialize_buffer(int bufferSize) {
    // Create shared memory
    int shm_id = shmget(shm_key, sizeof(buffer), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    // Attach to the shared memory
    buffer* b = (buffer*)shmat(shm_id, nullptr, 0);
    if (b == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    // Initialize buffer structure
    b->size = bufferSize;
    b->inIndex = 0;
    b->outIndex = 0;

    // Initialize semaphores
    sem_init(&b->e, 1, bufferSize);  // Empty slots semaphore
    sem_init(&b->mutex, 1, 1);       // Mutex semaphore
    sem_init(&b->n, 1, 0);           // Full slots semaphore

    return b;
}


buffer* attach_buffer() {
    // Get the shared memory ID
    int shm_id = shmget(shm_key, sizeof(buffer), 0666); // Use the same key and size as the producer
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    // Attach to the shared memory
    buffer* b = (buffer*)shmat(shm_id, nullptr, 0);
    if (b == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    return b; // Return the pointer to the buffer
}


void cleanup_buffer(buffer* b) {
    sem_destroy(&b->e);
    sem_destroy(&b->n);
    sem_destroy(&b->mutex);
    shmdt(b);
}

/*
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
*/
#endif