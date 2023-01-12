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

int registerPub(const char* pipeName, char* boxName);

int registerSub(const char* pipeName, const char* boxName);

int createBox(const char* pipeName, const char* boxName);

int destroyBox(const char* pipeName, const char* boxName);

int listBoxes();

#endif