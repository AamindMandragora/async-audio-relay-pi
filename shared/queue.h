#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define QUEUE_SIZE 32

//Taking from critical concurrency lab
typedef struct queue{
    void * q_buffer[QUEUE_SIZE];
    int in;
    int out;
    pthread_mutex_t lock;
    sem_t countsem, spacesem;
} queue;

queue * queue_init();

void queue_destroy(queue * q);

void queue_push(queue * q, void *element);


void *queue_pull(queue * q);