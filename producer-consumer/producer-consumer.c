#include "producer-consumer.h"

#include <stdlib.h>

#include "../utils/utils.h"

int pcq_create(pc_queue_t *queue, size_t capacity) {
  queue = (pc_queue_t *)malloc(sizeof(queue));
  if (queue == NULL) {
    perror("alloc_queue_error");
    exit(EXIT_FAILURE);
  }

  // ? Maybe usar algo que não char
  queue->pcq_buffer = (char **)malloc(sizeof(char) * capacity);
  if (queue == NULL) {
    perror("alloc_pcq_buffer_error");
    exit(EXIT_FAILURE);
  }

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

  free(queue);

  return 0;
}

int pcq_enqueue(pc_queue_t *queue, void *elem) {
  // TODO: Esperar que a queue não esteja cheia

  mutex_lock(&queue->pcq_head_lock);
  queue->pcq_buffer[queue->pcq_head] = NULL;  //! Mudar isto para o pcq
  queue->pcq_head++;

  if (queue->pcq_head == queue->pcq_capacity) {
    queue->pcq_head = 0;
  }

  mutex_unlock(&queue->pcq_head_lock);

  return 0;
}

void *pcq_dequeue(pc_queue_t *queue) {
  // TODO esperar que a queue não esteja vazia

  mutex_lock(&queue->pcq_tail_lock);
  queue->pcq_buffer[queue->pcq_tail] = NULL;
  queue->pcq_tail++;

  if (queue->pcq_tail == queue->pcq_capacity) {
    queue->pcq_tail = 0;
  }

  mutex_unlock(&queue->pcq_tail_lock);
}