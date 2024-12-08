#ifndef BUFFER_H
#define BUFFER_H

#include <semaphore.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#define SHARED_MEM_KEY 1234
#define SHARED_ID_KEY 5678
#define BUFFER_SIZE 10
#define ERROR -1

// Commodity structure
struct commodity {
    char name[32];
    double price;
    int id;
};

// Shared buffer structure
struct buffer {
    commodity items[BUFFER_SIZE];  // Array to store commodities
    int in_index;                 // Points to the next empty slot
    int out_index;                // Points to the next full slot
};

// Semaphore names
const char* SEM_EMPTY = "/sem_empty";
const char* SEM_FULL = "/sem_full";
const char* SEM_MUTEX = "/sem_mutex";
const char* SEM_ID_MUTEX = "/sem_id_mutex";

// Initialize shared memory and semaphores (parent process)
void initialize_shared_resources() {
    int shm_id = shmget(SHARED_MEM_KEY, sizeof(buffer), IPC_CREAT | 0666);
    if (shm_id == ERROR) {
        perror("shmget failed");
        exit(1);
    }

    buffer* buf = (buffer*)shmat(shm_id, nullptr, 0);
    if (buf == (void*)ERROR) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize the buffer
    memset(buf, 0, sizeof(buffer));
    shmdt(buf);

    // Initialize shared ID counter
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
    *id_counter = 1; // Start IDs from 1
    shmdt(id_counter);

    // Initialize semaphores
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_ID_MUTEX);

    sem_open(SEM_EMPTY, O_CREAT, 0644, BUFFER_SIZE);  // Initially BUFFER_SIZE empty slots
    sem_open(SEM_FULL, O_CREAT, 0644, 0);            // Initially no full slots
    sem_open(SEM_MUTEX, O_CREAT, 0644, 1);           // Mutex starts unlocked
    sem_open(SEM_ID_MUTEX, O_CREAT, 0644, 1);        // Mutex for ID counter
}

// Attach to shared memory
buffer* attach_to_buffer() {
    int shm_id = shmget(SHARED_MEM_KEY, sizeof(buffer), 0666);
    if (shm_id == ERROR) {
        perror("shmget failed");
        return nullptr;
    }

    buffer* buf = (buffer*)shmat(shm_id, nullptr, 0);
    if (buf == (void*)ERROR) {
        perror("shmat failed");
        return nullptr;
    }

    return buf;
}

// Get the next unique ID
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

    // Increment the ID atomically
    sem_t* id_mutex = sem_open(SEM_ID_MUTEX, 0);
    sem_wait(id_mutex);
    int id = (*id_counter)++;
    sem_post(id_mutex);
    sem_close(id_mutex);

    shmdt(id_counter);
    return id;
}

// Clean up shared resources
void cleanup_shared_resources() {
    shmctl(shmget(SHARED_MEM_KEY, sizeof(buffer), 0666), IPC_RMID, nullptr);
    shmctl(shmget(SHARED_ID_KEY, sizeof(int), 0666), IPC_RMID, nullptr);

    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_ID_MUTEX);
}

#endif
