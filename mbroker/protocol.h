#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "logging.h"
#include "message_box.h"
#include "operations.h"
#include "producer-consumer.h"
#include "utils.h"

typedef struct Registry_Protocol {
    __uint8_t code;
    char register_pipe_name[256];
    char box_name[32];
} Registry_Protocol;

typedef struct Box_Protocol {
    __uint8_t code;
    __int32_t response;
    char error_message[1024];
} Box_Protocol;

typedef struct Message_Protocol {
    char message[1024];
    __uint8_t code;
} Message_Protocol;

void thread_sig_handler(int sig);

int register_pub(const char *pipe_name, char *box_name);

int register_sub(const char *pipe_name, const char *box_name);

int create_box(const char *pipe_name, const char *box_name);

int destroy_box(const char *pipe_name, const char *box_name);

int send_list_boxes_protocol(const char *pipe_name);

#endif