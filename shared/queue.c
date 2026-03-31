//This code was adapted from the 341 textbook

#include "queue.h"
#include "stdlib.h"

queue *  queue_init() {
    queue * rQueue = malloc(sizeof(queue));
    rQueue->in = 0;
    rQueue->out = 0;
    pthread_mutex_init(&(rQueue->lock), NULL);
    sem_init(&(rQueue->countsem), 0 ,0);
    sem_init(&(rQueue->spacesem), 0 ,QUEUE_SIZE);
}

void queue_destroy(queue * q) {
    sem_destroy(&(q->countsem));
    sem_destroy(&(q->spacesem));    
    pthread_mutex_destroy(&(q->lock));
}

//Element is ptr to element on the heap
void queue_push(queue * q, void *element) {
  sem_wait( &(q->spacesem) );
  pthread_mutex_lock(&(q->lock));
  q->q_buffer[ ((q->in)++) & (QUEUE_SIZE-1) ] = element;
  p_m_unlock(&(q->lock));
  sem_post(&(q->countsem));

}

void *queue_pull(queue * q){
  sem_wait( &(q->countsem) );
  pthread_mutex_lock(&(q->lock));
  void * outEle = q->q_buffer[((q->out++)) & (QUEUE_SIZE-1)];
  p_m_unlock(&(q->lock));
  sem_post(&(q->spacesem));
  return outEle;
}