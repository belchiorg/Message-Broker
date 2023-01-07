#ifndef __UTILS_H__
#define __UTILS_H__

#include <pthread.h>
#include <stdlib.h>

#define MAX_BLOCK_LEN 1024
#define PIPE_NAME_SIZE 256
#define BOX_NAME_SIZE 32

void mutex_lock(pthread_mutex_t *lock);

void mutex_unlock(pthread_mutex_t *lock);

void mutex_destroy(pthread_mutex_t *lock);

void mutex_init(pthread_mutex_t *lock);

#endif