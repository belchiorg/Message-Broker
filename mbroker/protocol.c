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

int register_pub(const char* pipe_name, char* box_name, Box_Node* box_list) {
  int boxFd = tfs_open(box_name, TFS_O_APPEND);
  if ((boxFd) < 0) {
    perror("Error while opening box");
    exit(EXIT_FAILURE);
  }

  // TODO: Verify if there are any publishers
  Message_Box* box = find_message_box(box_name, box_list);
  if (box->n_publishers > 0) {
    exit(EXIT_FAILURE);
  }
  box->n_publishers++;

  //* unlink(pipe_name); -> passou para o publisher

  if (mkfifo(pipe_name, 0777) < 0) {
    perror("Error while creating fifo");
    exit(EXIT_FAILURE);
  }

  int sessionFd;
  if ((sessionFd = open(pipe_name, O_RDONLY)) < 0) {
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

int register_sub(const char* pipe_name, const char* box_name,
                 Box_Node* box_list) {
  int boxFd = tfs_open(box_name, 0);
  if (boxFd < 0) {
    perror("Error while creating/opening box");
    exit(EXIT_FAILURE);
  }

  int sessionFd;
  if ((sessionFd = open(pipe_name, O_WRONLY)) < 0) {
    perror("Error while opening fifo");
    exit(EXIT_FAILURE);
  }

  Message_Box* box = find_message_box(box_name, box_list);
  box->n_subscribers++;

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

    memset(message, 0, MESSAGE_SIZE);
  }

  close(boxFd);
  close(sessionFd);

  return 0;
}

int create_box(const char* pipe_name, const char* box_name,
               Box_Node* box_list) {
  Box_Protocol* response = (Box_Protocol*)malloc(sizeof(Box_Protocol));

  response->code = 4;

  if (find_message_box(box_name, box_list) != NULL) {
    response->response = -1;
    strcpy(response->error_message, "Message box already exists");
  } else {
    int boxfd;
    if ((boxfd = tfs_open(box_name, TFS_O_CREAT)) < 0) {
      response->response = -1;
      strcpy(response->error_message, "Error while creating box");
    } else {
      response->response = 0;
      memset(response->error_message, 0, strlen(response->error_message));
    }
    tfs_close(boxfd);
  }

  int fd = open(pipe_name, O_WRONLY);

  if (write(fd, response, sizeof(Box_Protocol)) < 0) {
    free(response);
    perror("Error while writing in manager fifo");
    exit(EXIT_FAILURE);
  }

  add_box(box_name, box_list);

  puts("ey");

  close(fd);
  free(response);
  return 0;
}

int destroy_box(const char* pipe_name, const char* box_name,
                Box_Node* box_list) {
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

  remove_box(box_name, box_list);

  close(file);

  free(response);

  return 0;
}

int send_list_boxes(const char* pipe_name, Box_Node* box_list) {
  Box_Node* ptr = box_list;

  fprintf(stdout, "ERROR %s\n", box_list->box->box_name);

  int pipe_fd = open(pipe_name, O_WRONLY);
  if (pipe_fd < 0) {
    perror("Error while opening Manager Fifo in list_boxes");
    exit(EXIT_FAILURE);
  }

  while (ptr != NULL) {
    Message_Box* box = ptr->box;
    fprintf(stdout, "ERROR %s\n", box->box_name);
    if (write(pipe_fd, box, sizeof(Message_Box)) < 0) {
      perror("Error while writing in manager Fifo");
      exit(EXIT_FAILURE);
    }
    ptr = ptr->next;
  }

  close(pipe_fd);

  return 0;
}
