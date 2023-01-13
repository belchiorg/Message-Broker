#include "protocol.h"

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

int registerPub(const char* pipeName, char* boxName) {
  int boxFd = tfs_open(boxName, TFS_O_APPEND);
  if ((boxFd) < 0) {
    perror("Error while opening box");
    exit(EXIT_FAILURE);
  }

  // TODO: Verify if there are any publishers
  if (0) {
    exit(EXIT_FAILURE);
  }

  unlink(pipeName);

  if (mkfifo(pipeName, 0777) < 0) {
    perror("Error while creating fifo");
    exit(EXIT_FAILURE);
  }

  int sessionFd;
  if ((sessionFd = open(pipeName, O_RDONLY)) < 0) {
    perror("Error while opening fifo");
    exit(EXIT_FAILURE);
  }

  char message[MESSAGE_SIZE];
  ssize_t n;

  while ((n = read(sessionFd, message, MESSAGE_SIZE)) > 0) {
    //* Reads what is in the fifo

    if (tfs_write(boxFd, message, (size_t)n) <= 0) {
      //* Writes it to the file System
      perror("Error while writing in fifo");
      exit(EXIT_FAILURE);
    }

    memset(message, 0, MESSAGE_SIZE);
  }

  tfs_close(boxFd);

  return 0;
}

int registerSub(const char* pipeName, const char* boxName) {
  int boxFd = tfs_open(boxName, 0);
  if (boxFd < 0) {
    perror("Error while creating/opening box");
    exit(EXIT_FAILURE);
  }

  int sessionFd;
  if ((sessionFd = open(pipeName, O_WRONLY)) < 0) {
    perror("Error while opening fifo");
    exit(EXIT_FAILURE);
  }

  char message[MESSAGE_SIZE];
  ssize_t n;

  if (tfs_read(boxFd, message, MESSAGE_SIZE) > 0) {
    //* Sends byte Code
    //! Passar esta merda para struct

    // if (write(sessionFd, "10|", 3) < 0) {
    //   //* Writes it to fifo
    //   perror("Error while writing in fifo");
    //   exit(EXIT_FAILURE);
    // }

    if (write(sessionFd, message, MESSAGE_SIZE) < 0) {
      //* Writes it to fifo
      perror("Error while writing in fifo");
      exit(EXIT_FAILURE);
    }
  }

  while ((n = tfs_read(boxFd, message, MESSAGE_SIZE)) > 0) {
    //* Reads what is in the file

    if (write(sessionFd, message, MESSAGE_SIZE) < 0) {
      //* Writes it to fifo
      perror("Error while writing in fifo");
      exit(EXIT_FAILURE);
    }
  }

  close(boxFd);
  close(sessionFd);

  return 0;
}

int createBox(const char* pipeName, const char* boxName) {
  Box_Protocol* response = (Box_Protocol*)malloc(sizeof(Box_Protocol));

  response->code = 4;

  int boxfd;

  if ((boxfd = tfs_open(boxName, TFS_O_CREAT)) < 0) {
    response->response = -1;
    strcpy(response->error_message, "Error while creating box");
  } else {
    response->response = 0;
    memset(response->error_message, 0, strlen(response->error_message));
  }

  fprintf(stdout, "box: %s\n", boxName);
  fprintf(stdout, "pipe: %s\n", pipeName);

  int fd = open(pipeName, O_WRONLY);

  fprintf(stdout, "file: %d\n", fd);
  puts("-----------------------");

  if (write(fd, response, sizeof(Box_Protocol)) < 0) {
    free(response);
    perror("Error while writing in manager fifo");
    exit(EXIT_FAILURE);
  }

  close(fd);
  free(response);
  tfs_close(boxfd);
  return 0;
}

int destroyBox(const char* pipeName, const char* boxName) {
  (void)pipeName;
  (void)boxName;
  Box_Protocol* response = (Box_Protocol*)malloc(sizeof(Box_Protocol));

  response->code = 6;

  if (tfs_unlink(boxName) == -1) {
    response->response = -1;
    strcpy(response->error_message, "Error while deleting boxName");
  } else {
    response->response = 0;
    memset(response->error_message, 0, strlen(response->error_message));
  }

  unlink(pipeName);

  mkfifo(pipeName, 0777);

  int file = open(pipeName, O_WRONLY);

  if (write(file, response, sizeof(Box_Protocol)) < -1) {
    free(response);
    perror("Error while writting in manager fifo");
    exit(EXIT_FAILURE);
  }

  free(response);

  return 0;
}

int listBoxes() {
  // TODO: Implement Me
  fprintf(stdout, "Isto é uma caixa que levou print uwu");
  return -1;
}
