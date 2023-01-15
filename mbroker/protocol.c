#include "protocol.h"

#include <fcntl.h>
#include <pthread.h>
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

int register_pub(const char *pipe_name, char *box_name) {
  Message_Box *box = find_message_box(box_name);
  if ((box == NULL)) {
    fprintf(stderr, "Err: message box does not exist");
    return -1;
  }
  if (box->n_publishers > 0) {
    fprintf(stderr, "Err: message box already has a publisher connected");
    return -1;
  }

  int box_fd = tfs_open(box_name, TFS_O_APPEND);
  if ((box_fd) < 0) {
    fprintf(stderr, "Err: couldn't open message box");
    return -1;
  }
  box->n_publishers++;

  int session_fd;
  if ((session_fd = open(pipe_name, O_RDONLY)) < 0) {
    box->n_publishers--;
    fprintf(stderr, "Err: couldn't open session fifo");
    return -1;
  }

  Message_Protocol *message =
      (Message_Protocol *)malloc(sizeof(Message_Protocol));

  ssize_t n;
  size_t len;

  memset(message->message, 0, 1024);

  // Cond Var para avisar que chegou novas cenas
  while ((read(session_fd, message, sizeof(Message_Protocol))) > 0) {
    mutex_lock(&box->n_messages_lock);
    box->n_messages++;
    mutex_unlock(&box->n_messages_lock);
    pthread_cond_broadcast(&box->box_cond_var);

    //* Reads what is in the fifo

    len = strlen(message->message) + 1;

    n = tfs_write(box_fd, message->message, len);

    if (n > 0) {
      box->box_size += (size_t)n;
    } else {
      break;
    }

    memset(message->message, 0, sizeof(message->message));
  }

  box->n_publishers--;

  close(session_fd);

  free(message);

  tfs_close(box_fd);

  return 0;
}

int register_sub(const char *pipe_name, const char *box_name) {
  int box_fd = tfs_open(box_name, 0);
  if (box_fd < 0) {
    fprintf(stderr, "Err: couldn't open message box");
    return -1;
  }

  int session_fd;
  if ((session_fd = open(pipe_name, O_WRONLY)) < 0) {
    fprintf(stderr, "Err: couldn't open session fifo");
    return -1;
  }

  Message_Box *box = find_message_box(box_name);
  ssize_t n;

  if (box == NULL) {
    fprintf(stderr, "Err: couldn't open message box");
    return -1;
  }

  mutex_lock(&box->n_messages_lock);
  int n_messages = box->n_messages++;
  mutex_unlock(&box->n_messages_lock);

  box->n_subscribers++;

  Message_Protocol *message =
      (Message_Protocol *)malloc(sizeof(Message_Protocol));

  message->code = 10;

  memset(message->message, 0, 1024);
  if (tfs_read(box_fd, message->message, 1024) > 0) {
    write(session_fd, message, sizeof(Message_Protocol));

    memset(message->message, 0, 1024);
  }

  while (1) {
    mutex_lock(&box->n_messages_lock);
    while (n_messages == box->n_messages) {
      pthread_cond_wait(&box->box_cond_var, &box->n_messages_lock);
    }
    mutex_unlock(&box->n_messages_lock);

    n = tfs_read(box_fd, message->message, 1024);
    if (n < 0) {
      break;
    }
    n_messages++;
    if (n == 0) continue;
    if (write(session_fd, message, sizeof(Message_Protocol)) <= 0) {
      //* If write is < 0, than the pipe is now closed
      break;
    }
    memset(message->message, 0, 1024);
  }

  free(message);

  tfs_close(box_fd);
  close(session_fd);

  return 0;
}

int create_box(const char *pipe_name, const char *box_name) {
  Box_Protocol *response = (Box_Protocol *)malloc(sizeof(Box_Protocol));
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
  if (fd < 0) {
    fprintf(stderr, "Err: couldn't open session fifo");
    return -1;
  }

  __uint8_t code = 4;

  if (write(fd, &code, 1) < 0) {
    free(response);
    fprintf(stderr, "Error while writing in manager fifo");
    return -1;
  }

  if (write(fd, response, sizeof(Box_Protocol)) < 0) {
    free(response);
    fprintf(stderr, "Error while writing in manager fifo");
    return -1;
  }

  close(fd);
  free(response);
  return 0;
}

int destroy_box(const char *pipe_name, const char *box_name) {
  Box_Protocol *response = (Box_Protocol *)malloc(sizeof(Box_Protocol));

  response->code = 6;

  if (tfs_unlink(box_name) == -1) {
    response->response = -1;
    strcpy(response->error_message, "Error while deleting box_name");
  } else {
    response->response = 0;
    memset(response->error_message, 0, strlen(response->error_message));
  }

  int file = open(pipe_name, O_WRONLY);
  if (file < 0) {
    fprintf(stderr, "Err: couldn't open session fifo");
    return -1;
  }

  if (write(file, response, sizeof(Box_Protocol)) < -1) {
    free(response);
    fprintf(stderr, "Error while writing in manager fifo");
    return -1;
  }

  remove_box(box_name);

  close(file);

  free(response);

  return 0;
}

int send_list_boxes(const char *pipe_name) {
  int pipe_fd = open(pipe_name, O_WRONLY);
  if (pipe_fd < 0) {
    fprintf(stderr, "Error while opening Manager Fifo in list_boxes");
    return -1;
  }

  send_list_boxes_from_other_file(pipe_fd);

  close(pipe_fd);

  return 0;
}
