#include "registry.h"

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
  int boxFd = tfs_open(boxName, TFS_O_APPEND | TFS_O_CREAT);
  if ((boxFd) < 0) {
    perror("Error while opening box");
    exit(EXIT_FAILURE);
  }

  // TODO: Verify if the box exists and if there are any other publishers
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

  ssize_t n;

  while ((n = read(sessionFd, message, MESSAGE_SIZE)) > 0) {
    fprintf(stdout, "%s", message);
    //* Reads what is in the fifo

    if (tfs_write(boxFd, message, MESSAGE_SIZE) <= 0) {
      //* Writes it to the file System
      perror("Error while writing in fifo");
      exit(EXIT_FAILURE);
    }

    sleep(1);  //! espera ativa :D
  }

  memset(message, 0, MESSAGE_SIZE);

  tfs_read(boxFd, message, MESSAGE_SIZE);

  printf("%s", message);
  puts("hey");

  return 0;
}

int registerSub(const char* pipeName, const char* boxName) {
  int boxFd = tfs_open(boxName, TFS_O_TRUNC);
  if (boxFd < 0) {
    perror("Error while creating/opening box");
    exit(EXIT_FAILURE);
  }

  if (mkfifo(pipeName, 0640) < 0) {
    perror("Error while creating fifo");
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
    if (write(sessionFd, "10|", 3) < 0) {
      //* Writes it to fifo
      perror("Error while writing in fifo");
      exit(EXIT_FAILURE);
    }
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

    // sleep(1);  //! espera ativa :D
  }

  close(boxFd);
  close(sessionFd);

  return 0;
}

int createBox(const char* pipeName, const char* boxName) {
  (void)pipeName;
  (void)boxName;
  // TODO: Implement Me
  return -1;
}

int destroyBox(const char* pipeName, const char* boxName) {
  (void)pipeName;
  (void)boxName;
  // TODO: Implement Me
  return -1;
}

int listBoxes() {
  // TODO: Implement Me
  fprintf(stdout, "Isto é uma caixa que levou print uwu");
  return -1;
}

//? Codigo daqui para cima talvez possa ser colocado noutro ficheiro :D