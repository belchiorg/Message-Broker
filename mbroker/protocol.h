#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../fs/operations.h"
#include "../producer-consumer/producer-consumer.h"
#include "../utils/logging.h"
#include "../utils/utils.h"
#include "message_box.h"

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

int register_pub(const char* pipeName, char* boxName, Box_Node* box_list);

int register_sub(const char* pipeName, const char* boxName, Box_Node* box_list);

int create_box(const char* pipeName, const char* boxName, Box_Node* box_list);

int destroy_box(const char* pipeName, const char* boxName, Box_Node* box_list);

int send_list_boxes(const char* pipe_name, Box_Node* box_list);

#endif