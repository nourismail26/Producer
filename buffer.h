#ifndef BUFFER_H
#define BUFFER_H
#include <semaphore.h>
#include <string>
#include <cstddef>
#include <sys/shm.h>
#define shm_key 1234
#define ERROR -1

#include <sys/mman.h>

struct commodity  {
    std::string name;
    double price;
};
struct buffer { 
    commodity * inBuff;      //points to first empty place                
    commodity * outBuff = NULL;      //points to first full place          
    int size;  //actual size of the buffer
    sem_t *e ;
    sem_t *n ;
    sem_t *mutex ;
};

int get_shared_buffer(int size){ //returns the buffer id
    key_t key = shm_key;
    int buffer_id = shmget(key,size,0666 | IPC_CREAT); 
        //key -> unique & identified by the user 
        //buffer_id -> unique, returned by shmget & assigned by os  
    return buffer_id;
}

buffer* attach_buffer(int size){    //attach the producer to buffer
    int shared_buffer_id = get_shared_buffer(size);
    buffer *result;

    if (shared_buffer_id == ERROR){
        return NULL;
    }
    result = (buffer*) shmat(shared_buffer_id, NULL,0 );
    if (result == (buffer *)ERROR){
        return NULL;
    }
    return result;

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