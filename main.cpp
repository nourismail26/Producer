#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "buffer.h"//include 'interface'

int main() 
{
    const char* shm_name = "/buffer";  

     buffer* shared_buffer = attach_buffer_to_shm(shm_name);
    if (shared_buffer == nullptr) {
        std::cerr << "Error attaching to shared memory!" << std::endl;
        return -1;
    }

     sem_init(&shared_buffer->mutex, 1, 1);//mutex initialized for mutual exclusion
    sem_init(&shared_buffer->empty, 1, BUFFERSIZE);//all slots are initially empty
    sem_init(&shared_buffer->full, 1, 0);//no commodities produced

    //fork producer and consumer processes then  wait 
    pid_t producer_pid = fork();
    if (producer_pid == 0) {
      producer(shared_buffer);
        return 0;
    }

    pid_t consumer_pid = fork();
    if (consumer_pid == 0) {
      consumer(shared_buffer);
        return 0;
    }
     waitpid(producer_pid, nullptr, 0);
    waitpid(consumer_pid, nullptr, 0);

    sem_destroy(&shared_buffer->mutex);
    sem_destroy(&shared_buffer->empty);
    sem_destroy(&shared_buffer->full);
    
    return 0;
}
