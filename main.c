#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define SIZE_BLOCK 5000  
#define MAX_SIZE_SAMPLE 500000L

typedef struct {
    long long t;
    long long interval;
} Record;

/* --- Shared Buffer --- */
Record shared_buffer[SIZE_BLOCK];
int shared_count = 0;
long global_X = 1000000;
int flag_x_changed = 0;
int flag_newline = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* ================= SAMPLE THREAD ================= */
void *sample_thread(void *arg) {
    Record local_buffer[SIZE_BLOCK];
    int local_idx = 0;
    long long prev_time = 0;
    long long total_collected = 0;
    long long time_now = 0;
    long current_x = 0;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    while (1) {
        pthread_mutex_lock(&lock);
        if (flag_x_changed) 
        {
            // Reset variables
            total_collected = 0;
            flag_x_changed = 0;
            flag_newline = 1; 
            prev_time = 0;
        }
        // Get current X
        current_x = global_X;
        pthread_mutex_unlock(&lock);

        ts.tv_nsec += current_x;
        while (ts.tv_nsec >= 1000000000LL) 
        {
            ts.tv_nsec -= 1000000000LL;
            ts.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);

        struct timespec actual_ts;
        clock_gettime(CLOCK_MONOTONIC, &actual_ts);

        // Check if not exceeded max sample size
        if (total_collected < MAX_SIZE_SAMPLE) {
            time_now = (long long)actual_ts.tv_sec * 1000000000LL + actual_ts.tv_nsec;
            
            local_buffer[local_idx].t = time_now;
            local_buffer[local_idx].interval = (prev_time == 0) ? current_x : (time_now - prev_time);
            prev_time = time_now;
            
            local_idx++;
            total_collected++;

            // Update shared buffer
            if (local_idx >= SIZE_BLOCK || total_collected >= MAX_SIZE_SAMPLE) 
            {
                pthread_mutex_lock(&lock);
                while (shared_count > 0) 
                {
                    pthread_cond_wait(&cond, &lock);
                }
                
                memcpy(shared_buffer, local_buffer, sizeof(Record) * local_idx);
                shared_count = local_idx;
                
                pthread_cond_signal(&cond);
                pthread_mutex_unlock(&lock);
                local_idx = 0; // Reset index
            }
        }
    }
}

/* ================= LOGGING THREAD ================= */
void *logging_thread(void *arg) 
{
    FILE *f = fopen("time_and_interval.txt", "w");
    if (f == NULL) 
    {
        printf("Cannot open file");
        return NULL;
    }

    Record buffer[SIZE_BLOCK];
    
    while (1) 
    {
        pthread_mutex_lock(&lock);
        while (shared_count == 0) 
        {
            pthread_cond_wait(&cond, &lock);
        }
        
        if (flag_newline) 
        {
            fprintf(f, "\n");
            flag_newline = 0;
        }

        // Take data from shared buffer
        int count = shared_count;
        memcpy(buffer, shared_buffer, sizeof(Record) * count);
        shared_count = 0;
        
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);

        // Write to file
        for (int i = 0; i < count; i++) 
        {
            fprintf(f, "%lld %lld\n", buffer[i].t, buffer[i].interval);
        }
        fflush(f); 
    }
    fclose(f);
    return NULL;
}

/* ================= INPUT THREAD ================= */
void *input_thread(void *arg) {
    long newX;

    while (1)
    {
        FILE *f = fopen("freq.txt", "r");
        if (f) 
        {
            if (fscanf(f, "%ld", &newX) == 1) 
            {
                pthread_mutex_lock(&lock);
                if (newX != global_X) 
                {
                    printf("Update X = %ld ns\n", newX);
                    global_X = newX;
                    flag_x_changed = 1;
                }
                pthread_mutex_unlock(&lock);
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