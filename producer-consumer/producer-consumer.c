#include "producer-consumer.h"

#include <stdio.h>
#include <stdlib.h>

#include "../mbroker/protocol.h"
#include "../utils/utils.h"

int pcq_create(pc_queue_t *queue, size_t capacity) {
  queue->pcq_buffer = malloc(sizeof(Registry_Protocol) * capacity);
  if (queue == NULL) {
    fprintf(stderr, "alloc_pcq_buffer_error");
    exit(EXIT_FAILURE);
  }

  pthread_cond_init(&queue->pcq_pusher_condvar, 0);

  pthread_cond_init(&queue->pcq_popper_condvar, 0);

  mutex_init(&queue->pcq_current_size_lock);

  mutex_init(&queue->pcq_head_lock);

  mutex_init(&queue->pcq_tail_lock);

  mutex_init(&queue->pcq_pusher_condvar_lock);

  mutex_init(&queue->pcq_popper_condvar_lock);

  queue->pcq_capacity = capacity;
  queue->pcq_current_size = 0;
  queue->pcq_head = 0;
  queue->pcq_tail = 0;

  return 0;
}

int pcq_destroy(pc_queue_t *queue) {
  free(queue->pcq_buffer);

  mutex_destroy(&queue->pcq_current_size_lock);

  mutex_destroy(&queue->pcq_head_lock);

  mutex_destroy(&queue->pcq_tail_lock);

  mutex_destroy(&queue->pcq_pusher_condvar_lock);

  mutex_destroy(&queue->pcq_popper_condvar_lock);

  pthread_cond_destroy(&queue->pcq_pusher_condvar);

  pthread_cond_destroy(&queue->pcq_popper_condvar);

  return 0;
}

int pcq_enqueue(pc_queue_t *queue, void *elem) {
  mutex_lock(&queue->pcq_pusher_condvar_lock);

  while (queue->pcq_current_size == queue->pcq_capacity) {
    pthread_cond_wait(&queue->pcq_pusher_condvar,
                      &queue->pcq_pusher_condvar_lock);
  }

  mutex_unlock(&queue->pcq_pusher_condvar_lock);

  mutex_lock(&queue->pcq_tail_lock);
  mutex_lock(&queue->pcq_current_size_lock);
  queue->pcq_buffer[queue->pcq_tail] = elem;
  queue->pcq_tail++;

  if (queue->pcq_tail == queue->pcq_capacity) {
    queue->pcq_tail = 0;
  }

  queue->pcq_current_size++;
  fprintf(stdout, "%lu", queue->pcq_current_size);
  mutex_unlock(&queue->pcq_current_size_lock);
  mutex_unlock(&queue->pcq_tail_lock);

  pthread_cond_signal(&queue->pcq_popper_condvar);

  return 0;
}

void *pcq_dequeue(pc_queue_t *queue) {
  mutex_lock(&queue->pcq_popper_condvar_lock);

  while (queue->pcq_current_size == 0) {
    pthread_cond_wait(&queue->pcq_popper_condvar,
                      &queue->pcq_popper_condvar_lock);
  }

  mutex_unlock(&queue->pcq_popper_condvar_lock);

  mutex_lock(&queue->pcq_head_lock);
  mutex_lock(&queue->pcq_current_size_lock);

  Registry_Protocol *registry = queue->pcq_buffer[queue->pcq_head];

  queue->pcq_buffer[queue->pcq_head] = NULL;
  queue->pcq_head++;

  if (queue->pcq_head == queue->pcq_capacity) {
    queue->pcq_head = 0;
  }

  queue->pcq_current_size--;
  mutex_unlock(&queue->pcq_head_lock);
  mutex_unlock(&queue->pcq_current_size_lock);

  pthread_cond_signal(&queue->pcq_pusher_condvar);

  return registry;
}