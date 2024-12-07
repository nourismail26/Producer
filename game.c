#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GRID_SIZE 20
#define NUM_THREADS 4
#define GENERATIONS 32

int grid[GRID_SIZE][GRID_SIZE];
int next_grid[GRID_SIZE][GRID_SIZE];
pthread_barrier_t barrier;

typedef struct {
    int start_row;
    int end_row;
    int thread_num;
} ThreadData;

void print_grid()
{
    system("clear");
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            if (grid[i][j] == 1)
            {
                printf("# ");
            }
            else
            {
                printf("  ");
            }
        }
        printf("\n");
    }
    usleep(500000);
}

// Function to compute next generation of Game of Life
void *compute_next_gen(void *arg)
{
    ThreadData* data = (ThreadData*) arg;
    
    int start = data->start_row;
    int end = data->end_row;
    
    for (int row = start; row < end; row++)
    {
        for (int col = 0; col < GRID_SIZE; col++)
        {

            int live_neighbours = 0;
            // count number of live neighbours
            for (int i = -1; i <= 1; i++)
            {
                for (int j = -1; j <= 1; j++)
                {
                    if (i == 0 && j == 0)
                        continue; 

                    int n_row=row+i;
                    int n_col=col+j;
                    //make sure en el position not out of bounds 
                    if(n_row>=0 && n_row< GRID_SIZE && n_col>=0 && n_col< GRID_SIZE){
                       live_neighbours += grid[row+i][col+j];
                    }
                }
            }
            // Apply rules of the game

            // Birth Rule
            if (grid[row][col] == 0 && live_neighbours == 3)
            {
                next_grid[row][col] = 1;
            }

            // Survival Rule
            else if (grid[row][col] == 1 && (live_neighbours == 3 || live_neighbours == 2))
            {
                next_grid[row][col] = 1;
            }
             // Death Rule
            else if (grid[row][col] == 1 && (live_neighbours < 2 || live_neighbours > 3))
            {
                next_grid[row][col] = 0;
            }
            else
            {
                next_grid[row][col]=grid[row][col];
            }

        }
    }
    pthread_barrier_wait(&barrier);

    //next grid temporarily stores the changes 
    //fa lazem n copy el changes fel grid el asleya 
    //lama el thread teb2a 0
    // lw ma3malnash keda
    // el threads kolaha momken tehawel ta3mel keda 
    // w yekhosho feh race condition 
    
    if (data->thread_num == 0) {
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            grid[row][col] = next_grid[row][col];
        }
    }
}





}

void initialize_grid(int grid[GRID_SIZE][GRID_SIZE])
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            grid[i][j] = 0; // Set every cell to 0 (dead)
        }
    }
}

void initialize_patterns(int grid[GRID_SIZE][GRID_SIZE])
{
    initialize_grid(grid);

    // Initialize Still Life (Square) at top-left
    grid[1][1] = 1;
    grid[1][2] = 1;
    grid[2][1] = 1;
    grid[2][2] = 1;

    // Initialize Oscillator (Blinker) in the middle
    grid[5][6] = 1;
    grid[6][6] = 1;
    grid[7][6] = 1;

    // Initialize Spaceship (Glider) in the bottom-right
    grid[10][10] = 1;
    grid[11][11] = 1;
    grid[12][9] = 1;
    grid[12][10] = 1;
    grid[12][11] = 1;
}

int main()
{
    initialize_patterns(grid);
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);


    ThreadData thread_data[NUM_THREADS];
    pthread_t threads[NUM_THREADS];

    for (int generation = 0; generation < GENERATIONS; generation++) {
        
        for (int i = 0; i < NUM_THREADS; i++) {
            thread_data[i].start_row = i * (GRID_SIZE / NUM_THREADS);


            thread_data[i].end_row = (i + 1) * (GRID_SIZE / NUM_THREADS);

            thread_data[i].thread_num = i;

            pthread_create(&threads[i], NULL, compute_next_gen,&thread_data[i]);
        }

        
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(threads[i], NULL);
        }

        print_grid();
    }

    pthread_barrier_destroy(&barrier);
    return 0;
}

