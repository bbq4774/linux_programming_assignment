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
    struct timespec time;
    struct timespec sleep_time;

    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC, &time);

        long long time_ns = time.tv_sec * 1000000000LL + time.tv_nsec;

        pthread_mutex_lock(&lock);

        T = time_ns;
        pthread_cond_signal(&cond);

        long currentX = X;

        pthread_mutex_unlock(&lock);

        sleep_time.tv_sec = currentX / 1000000000;
        sleep_time.tv_nsec = currentX % 1000000000;

        nanosleep(&sleep_time, NULL);
    }
}

/* ================= LOGGING THREAD ================= */
void *logging_thread(void *arg)
{
    FILE *f = fopen("time_and_interval.txt", "w");

    if (f == NULL)
    {
        printf("Cannot open file\n");
        return NULL;
    }

    long lastX = X;
    while (1)
    {
        pthread_mutex_lock(&lock);

        while (T == prevT)
            pthread_cond_wait(&cond, &lock);

        long long interval = 0;

        if (prevT != 0)
            interval = T - prevT;

        if (X != lastX)
        {
            fprintf(f, "\n");
            lastX = X;
        }

        fprintf(f, "%lld %lld\n", T, interval);
        fflush(f);

        prevT = T;

        pthread_mutex_unlock(&lock);
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
                pthread_mutex_lock(&lock);

                if (newX != X)
                {
                    printf("Update X = %ld ns\n", newX);
                    X = newX;
                }

                pthread_mutex_unlock(&lock);
            }

            fclose(f);
        }

        sleep(1);
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