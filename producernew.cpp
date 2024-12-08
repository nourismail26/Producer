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
#include <time.h>
#include <sys/sem.h>
#define SHARED_MEM_KEY 2003
#define ERROR -1

struct sembuf waiting= {0, -1, SEM_UNDO}; //decrementing the semaphore
struct sembuf signaling = {0, 1, SEM_UNDO};  //incrementin the semaphore
int inBuff = 0; //global variable to track where the to place the commodity

struct commodity  {
    std::string name;
    double price;
}; 

double generate_price(double mean, double stddev){
    std::default_random_engine generator;
    std::normal_distribution<double> dist(mean, stddev);
    return dist(generator);
}

commodity produce(std::string Name , double mean , double stddev,std::string message){
    commodity c ;
    c.name = Name;
    c.price = generate_price(mean , stddev);
    message = "generating a new value of " + std::to_string(c.price) + "\n";
    return c;

}
void add_to_buffer(commodity c,commodity* buffer,int bufferSize,std::string message){
    message = "adding a new value to buffer " + std::to_string(c.price) + "\n";
    buffer[inBuff] = c;
    inBuff = (inBuff+1)%bufferSize;
}
void logMessage(std::string name,std::string message){

    timespec timeptr;
    clock_gettime(CLOCK_REALTIME, &timeptr);
    tm *time = localtime(&timeptr.tv_sec);
    char temp[30];
    strftime(temp,sizeof(temp) , "%m/%d/%Y %H:%M:%S" , time);
    std::string space =" ";
    std::string left ="[";
    message =  left+ temp +"]"+ space +name+ space+ message;
    std::cerr << message << std::endl;
}
void sleep(int sleepInterval){
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepInterval)); //to wait for a while before reproducing
}
int get_semaphore(int key) {  
    key_t k = ftok("for_the_key", key); 
    if (k == -1) {  
        perror("ftok");        
        return ERROR;     }    
    int semid = semget(k, 1, 0666); 
    if (semid == -1) {      
        perror("semget");        
        return ERROR;     }    
    return semid; 
}

int get_shared_buffer(int size){ //returns the buffer id
    key_t key = SHARED_MEM_KEY;
    int buffer_id = shmget(key,size,0666); 
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

int main(int argc, char* argv[]) {

    //checking for the number of arguments
    if (argc < 6) {
        std::cerr << "Usage: <program_name> <commodity> <mean> <stddev> <sleep interval> <size of buffer>\n";
        return 1;
    }

    std::string commodity_name = argv[1];
    double mean = std::stod(argv[2]);
    double stddev = std::stod(argv[3]);
    int sleepInterval = std::stoi(argv[4]); // in milliseconds
    int bufferSize = std::stoi(argv[5]);

    if (bufferSize <= 0) {
        std::cerr << "Buffer size must be a positive integer.\n";
        return 1;
    }

  //getting the semaphores  e->10 , mutex->20 , n->30
  int e = get_semaphore(10);  
  int mutex = get_semaphore(20);  
  int n = get_semaphore(30);

  if (e == ERROR || mutex == ERROR || n == ERROR) { 
    std::cerr << "Error accessing semaphores.\n"; return 1; 
  }

   //initialize buffer and an index -> for the first empty place
    commodity *buffer = NULL; 
 
   //attach to shared memory
    int buffSizeInBytes = bufferSize * sizeof(commodity);
    buffer = attach_to_buffer(buffSizeInBytes); //pointer to the first place in shared memory
 
    std::string message = ""; 

 while (true) {

        commodity c = produce(commodity_name,mean,stddev,message);
            logMessage(commodity_name,message);
        semop(e,&waiting,1); 
            message = "Trying to get mutex on shared buffer\n";
            logMessage(commodity_name,message);    
        semop(mutex,&waiting,1);  
        add_to_buffer(c,buffer,bufferSize,message);
            logMessage(commodity_name,message);   
        semop(mutex , &signaling,1);
        semop(n,&signaling,1);
            logMessage(commodity_name,message);
            message = "Sleeping for "+std::to_string(sleepInterval) + "\n";
        sleep(sleepInterval);
    }

 return 0;
}

