#include "queue.h"
#include "stdlib.h"

queue *queue_init() {
	queue *q = malloc(sizeof(queue));
	q->in = 0;
	q->out = 0;
	pthread_mutex_init(&(q->lock), NULL);
  	sem_init(&(q->countsem), 0, 0);
  	sem_init(&(q->spacesem), 0, QUEUE_SIZE);
  	return q;
}

void queue_destroy(queue *q) {
	sem_destroy(&(q->countsem));
	sem_destroy(&(q->spacesem));    
	pthread_mutex_destroy(&(q->lock));
}

void queue_push(queue *q, void *element) {
	sem_wait(&(q->spacesem));
	pthread_mutex_lock(&(q->lock));
	q->buffer[((q->in)++) & (QUEUE_SIZE-1)] = element;
	pthread_mutex_unlock(&(q->lock));
	sem_post(&(q->countsem));
}

void *queue_pull(queue *q){
	sem_wait(&(q->countsem));
	pthread_mutex_lock(&(q->lock));
	void *element = q->buffer[((q->out++)) & (QUEUE_SIZE-1)];
	pthread_mutex_unlock(&(q->lock));
	sem_post(&(q->spacesem));
	return element;
}