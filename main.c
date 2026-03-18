#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

long X = 1000000;
long long T = 0;
long long prevT = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* ================= SAMPLE THREAD ================= */
void *sample_thread(void *arg)
{
    struct timespec ts;
    long long next_sample_time;
    
    clock_gettime(CLOCK_MONOTONIC, &ts);
    next_sample_time = ts.tv_sec * 1000000000LL + ts.tv_nsec;

    long long now;
    long long currentX;
    while (1)
    {
        do {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            now = ts.tv_sec * 1000000000LL + ts.tv_nsec;
        } while (now < next_sample_time); 

        pthread_mutex_lock(&lock);
        
        T = now;
        currentX = X;
        
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);

        next_sample_time += currentX; 
    }
}

/* ================= LOGGING THREAD ================= */
#define MAX_SAMPLES_PER_X 500000L 
#define BUFFER_SIZE 10000L

typedef struct {
    long long t;
    long long interval;
} Record;

void *logging_thread(void *arg)
{
    FILE *f = fopen("time_and_interval.txt", "w");
    if (f == NULL) 
    {
        printf("Cannot open file");
        return NULL;
    }

    Record buffer[BUFFER_SIZE];
    int buf_idx = 0;
    long long samples_collected = 0;
    long lastX = X;
    
    long long local_T, local_prevT;
    long local_X;

    while (1)
    {
        pthread_mutex_lock(&lock);
        while (T == prevT)
            pthread_cond_wait(&cond, &lock);

        local_T = T;
        local_prevT = prevT;
        local_X = X;
        prevT = T; 
        pthread_mutex_unlock(&lock); 

        // Check if X has changed
        if (local_X != lastX) 
        {
            for (int i = 0; i < buf_idx; i++) 
            {
                fprintf(f, "%lld %lld\n", buffer[i].t, buffer[i].interval);
            }
            fprintf(f, "\n");
            
            // Reset
            buf_idx = 0;
            samples_collected = 0;
            lastX = local_X;
            fflush(f);
        }

        if (samples_collected < MAX_SAMPLES_PER_X) 
        {
            long long interval = (local_prevT == 0) ? 0 : (local_T - local_prevT);
            
            // Save to buffer
            buffer[buf_idx].t = local_T;
            buffer[buf_idx].interval = interval;
            buf_idx++;
            samples_collected++;

            // Flush buffer to file if full
            if (buf_idx >= BUFFER_SIZE) 
            {
                for (int i = 0; i < BUFFER_SIZE; i++) 
                {
                    fprintf(f, "%lld %lld\n", buffer[i].t, buffer[i].interval);
                }
                buf_idx = 0;
            }
        }
    }
    
    fclose(f);
    return NULL;
}

/* ================= INPUT THREAD ================= */
void *input_thread(void *arg)
{
    long newX;

    while (1)
    {
        FILE *f = fopen("freq.txt", "r");

        if (f)
        {
            if (fscanf(f, "%ld", &newX) == 1)
            {
                if (newX != X)
                {
                    printf("Update X = %ld ns\n", newX);
                    
                    pthread_mutex_lock(&lock);

                    X = newX;
                    
                    pthread_mutex_unlock(&lock);
                }
                
            }
            fclose(f);
        }
        sleep(5);
    }
}

/* ================= MAIN ================= */
int main()
{
    pthread_t sample, logging, input;

    /* Create threads */
    pthread_create(&sample, NULL, sample_thread, NULL);
    pthread_create(&logging, NULL, logging_thread, NULL);
    pthread_create(&input, NULL, input_thread, NULL);

    /* Wait for threads to terminate */
    pthread_join(sample, NULL);
    pthread_join(logging, NULL);
    pthread_join(input, NULL);

    return 0;
}