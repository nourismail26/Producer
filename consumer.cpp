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
#include <cmath>
#include <fcntl.h>
#include "buffer.h"
#define shm_key 1234
#define ERROR -1


void consume(commodity c,commodity *consumed){

}

commodity take_from_buffer(commodity* &outBuff, int size){
    commodity c = *outBuff;
    //outBuff = (outBuff + 1)%size;
    return c;
}

int main(int argc) {

    //attaching the consumer to the buffer
    commodity consumed[5]; // array to track the last five commodities
    int size = sizeof(commodity); //el heta deeh ghalat
    buffer *b = attach_buffer(size);  //fi attach buffer by ignore el size law el shared memory created asln
    sem_t *e = b->e;
    sem_t *mutex = b->mutex;
    sem_t *n = b->n;
    commodity *outBuff = b->outBuff;
    int n = b->size;

 while (true) {

        sem_wait(n);      
        sem_wait(mutex);  
        commodity c  = take_from_buffer(outBuff, n );
        sem_post(mutex);
        sem_post(e);
        consume(c,consumed); 
    }

    ///shmdt(mybuffer);
    sem_close(e);
    sem_close(n);
    sem_close(mutex);

 return 0;
}