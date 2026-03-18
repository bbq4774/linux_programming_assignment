#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define SIZE_BLOCK 50000 
#define MAX_SIZE_SAMPLE 500000L 

typedef struct {
    long long t;
    long long interval;
} Record;

/* --- Double Buffer --- */
Record buffer1[SIZE_BLOCK];
Record buffer2[SIZE_BLOCK];
Record *active_buffer = buffer1;
Record *full_buffer = NULL;

int active_idx = 0;
long global_X = 1000000;
long long total_collected = 0;
int flag_x_changed = 0;
int flag_newline = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* ================= SAMPLE THREAD ================= */
void *sample_thread(void *arg) {
    long long prev_time = 0;
    long current_x;
    long long current_total;
    long long time_now;

    struct timespec actual_ts;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    while (1) {
        pthread_mutex_lock(&lock);
        if (flag_x_changed) 
        {
            // Reset variables
            total_collected = 0;
            prev_time = 0;
            flag_x_changed = 0;
            flag_newline = 1;
        }
        current_x = global_X;
        current_total = total_collected;
        pthread_mutex_unlock(&lock);

        ts.tv_nsec += current_x;
        while (ts.tv_nsec >= 1000000000LL) 
        {
            ts.tv_nsec -= 1000000000LL;
            ts.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);

        // Check if not exceeded max sample size
        if (current_total < MAX_SIZE_SAMPLE) {
            clock_gettime(CLOCK_MONOTONIC, &actual_ts);
            time_now = (long long)actual_ts.tv_sec * 1000000000LL + actual_ts.tv_nsec;

            active_buffer[active_idx].t = time_now;
            active_buffer[active_idx].interval = (prev_time == 0) ? current_x : (time_now - prev_time);
            prev_time = time_now;
            active_idx++;

            // Update buffer
            if (active_idx >= SIZE_BLOCK || (current_total + active_idx) >= MAX_SIZE_SAMPLE) {
                pthread_mutex_lock(&lock);
                while (full_buffer != NULL) {
                    pthread_cond_wait(&cond, &lock);
                }
                
                full_buffer = active_buffer;
                active_buffer = (active_buffer == buffer1) ? buffer2 : buffer1;
                total_collected += active_idx;
                active_idx = 0;

                pthread_cond_signal(&cond);
                pthread_mutex_unlock(&lock);
            }
        }
    }
    return NULL;
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
    
    while (1) 
    {
        pthread_mutex_lock(&lock);
        while (full_buffer == NULL) 
        {
            pthread_cond_wait(&cond, &lock);
        }

        if (flag_newline) 
        {
            fprintf(f, "\n");
            flag_newline = 0;
        }

        Record *buffer_to_write = full_buffer;
        full_buffer = NULL;
        
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);

        for (int i = 0; i < SIZE_BLOCK; i++) 
        {
            fprintf(f, "%lld %lld\n", buffer_to_write[i].t, buffer_to_write[i].interval);
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