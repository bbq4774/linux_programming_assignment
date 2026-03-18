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

    while (1)
    {
        long long now;
        do {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            now = ts.tv_sec * 1000000000LL + ts.tv_nsec;
        } while (now < next_sample_time); 

        pthread_mutex_lock(&lock);
        
        T = now;
        long long currentX = X;
        
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);

        next_sample_time += currentX; 
    }
}

/* ================= LOGGING THREAD (OPTIMIZED) ================= */
void *logging_thread(void *arg)
{
    FILE *f = fopen("time_and_interval.txt", "w");
    if (f == NULL) {
        printf("Cannot open file\n");
        return NULL;
    }

    long lastX = X;
    long long local_T, local_prevT, local_interval;
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

        local_interval = (local_prevT == 0) ? 0 : (local_T - local_prevT);

        if (local_X != lastX) {
            fprintf(f, "\n");
            lastX = local_X;
        }

        fprintf(f, "%lld %lld\n", local_T, local_interval);
    }
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