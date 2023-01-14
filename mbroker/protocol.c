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
#include "message_box.h"

int register_pub(const char* pipe_name, char* box_name) {
  int boxFd = tfs_open(box_name, TFS_O_APPEND);
  if ((boxFd) < 0) {
    return -1;
  }

  Message_Box* box = find_message_box(box_name);
  if (box->n_publishers > 0) {
    exit(EXIT_FAILURE);
  }
  box->n_publishers++;

  sleep(1);

  int sessionFd;
  if ((sessionFd = open(pipe_name, O_RDONLY)) < 0) {
    box->n_publishers--;
    perror("Error while opening fifo");
    exit(EXIT_FAILURE);
  }

  Message_Protocol* message =
      (Message_Protocol*)malloc(sizeof(Message_Protocol));

  ssize_t n;
  size_t len;

  memset(message->message, 0, 1024);
  while ((read(sessionFd, message, sizeof(Message_Protocol))) > 0) {
    //* Reads what is in the fifo

    if (write(1, message->message, sizeof(message->message))) {
    }

    len = strlen(message->message) + 1;

    n = tfs_write(boxFd, message->message, len);

    if (n >= 0) {
      box->box_size += (size_t)n;
    }

    memset(message->message, 0, sizeof(message->message));
  }

  box->n_publishers--;

  close(sessionFd);

  free(message);

  tfs_close(boxFd);

  return 0;
}

int register_sub(const char* pipe_name, const char* box_name) {
  int boxFd = tfs_open(box_name, 0);
  if (boxFd < 0) {
    return -1;
  }

  int sessionFd;
  if ((sessionFd = open(pipe_name, O_WRONLY)) < 0) {
    perror("Error while opening fifo");
    exit(EXIT_FAILURE);
  }

  Message_Box* box = find_message_box(box_name);
  box->n_subscribers++;

  Message_Protocol* message =
      (Message_Protocol*)malloc(sizeof(Message_Protocol));

  message->code = 10;

  memset(message->message, 0, 1024);
  if (tfs_read(boxFd, message->message, 1024) > 0) {
    if (write(sessionFd, message, sizeof(Message_Protocol)) < 0) {
      //* Writes it to fifo
      free(message);
      perror("Error while writing in fifo");
      exit(EXIT_FAILURE);
    }

    memset(message->message, 0, 1024);
  }

  ssize_t n;

  while ((n = tfs_read(boxFd, message->message, 1024)) >= 0) {
    if (n == 0) continue;
    if (write(sessionFd, message, sizeof(Message_Protocol)) < 0) {
      //* Writes it to fifo
      free(message);
      perror("Error while writing in fifo");
      exit(EXIT_FAILURE);
    }
    memset(message->message, 0, 1024);
  }

  free(message);

  tfs_close(boxFd);
  close(sessionFd);

  return 0;
}

int create_box(const char* pipe_name, const char* box_name) {
  Box_Protocol* response = (Box_Protocol*)malloc(sizeof(Box_Protocol));

  response->code = 4;

  if (find_message_box(box_name) != NULL) {
    response->response = -1;
    strcpy(response->error_message, "Message box already exists");
  } else {
    int box_fd;
    if ((box_fd = tfs_open(box_name, TFS_O_CREAT)) < 0) {
      response->response = -1;
      strcpy(response->error_message, "Error while creating box");
    } else {
      response->response = 0;
      add_box(box_name, box_fd);
      memset(response->error_message, 0, strlen(response->error_message));
    }
  }

  int fd = open(pipe_name, O_WRONLY);

  __uint8_t code = 4;

  if (write(fd, &code, 1) < 0) {
    free(response);
    perror("Error while writing in manager fifo");
    exit(EXIT_FAILURE);
  }

  if (write(fd, response, sizeof(Box_Protocol)) < 0) {
    free(response);
    perror("Error while writing in manager fifo");
    exit(EXIT_FAILURE);
  }

  close(fd);
  free(response);
  return 0;
}

int destroy_box(const char* pipe_name, const char* box_name) {
  Box_Protocol* response = (Box_Protocol*)malloc(sizeof(Box_Protocol));

  response->code = 6;

  if (tfs_unlink(box_name) == -1) {
    response->response = -1;
    strcpy(response->error_message, "Error while deleting box_name");
  } else {
    response->response = 0;
    memset(response->error_message, 0, strlen(response->error_message));
  }

  int file = open(pipe_name, O_WRONLY);

  if (write(file, response, sizeof(Box_Protocol)) < -1) {
    free(response);
    perror("Error while writting in manager fifo");
    exit(EXIT_FAILURE);
  }

  remove_box(box_name);

  close(file);

  free(response);

  return 0;
}

int send_list_boxes(const char* pipe_name) {
  int pipe_fd = open(pipe_name, O_WRONLY);
  if (pipe_fd < 0) {
    perror("Error while opening Manager Fifo in list_boxes");
    exit(EXIT_FAILURE);
  }

  send_list_boxes_so_que_la_do_outro_ficheiro(pipe_fd);

  close(pipe_fd);

  return 0;
}
