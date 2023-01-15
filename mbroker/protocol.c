#include "protocol.h"

#include <fcntl.h>
#include <pthread.h>
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

/**
 * @brief Free any of the messages sent by user, any box of messages or any
 * request to create boxes and closes the file.
 *
 * @param sig signal received
 */
void thread_sig_handler(int sig) {
  (void)sig;

  write(1, "Exiting Thread", 15);
  exit(EXIT_SUCCESS);
}

/**
 * @brief Create a publisher that search by the right message in the box, open
 * the file and allocate memory for the pub. After that, read and keep all the
 * messages to write in the box while still have message sent.
 *
 * @param pipe_name client session pipe
 * @param prot_box_name message box in use
 */
int register_pub(const char *pipe_name, char *prot_box_name) {
  // Tries to get the message box
  Message_Box *prot_box = find_message_box(prot_box_name);
  if ((prot_box == NULL)) {
    fprintf(stderr, "Err: message prot_box does not exist");
    return -1;
  }

  // If there is already a publisher connected to the message box, returns and
  // frees the registry
  if (prot_box->n_publishers > 0) {
    fprintf(stderr, "Err: message prot_box already has a publisher connected");
    return -1;
  }

  // Opens the message box
  int prot_box_fd = tfs_open(prot_box_name, TFS_O_APPEND);
  if ((prot_box_fd) < 0) {
    fprintf(stderr, "Err: couldn't open message prot_box");
    return -1;
  }
  prot_box->n_publishers++;

  // Opens the client session pipe
  int prot_session_fd;
  if ((prot_session_fd = open(pipe_name, O_RDONLY)) < 0) {
    prot_box->n_publishers--;
    fprintf(stderr, "Err: couldn't open session fifo");
    return -1;
  }

  // Allocs memory for the message
  Message_Protocol *prot_message =
      (Message_Protocol *)malloc(sizeof(Message_Protocol));

  ssize_t n;
  size_t len;

  memset(prot_message->message, 0, 1024);

  while ((read(prot_session_fd, prot_message, sizeof(Message_Protocol))) > 0) {
    //* Reads new message from client session fifo

    mutex_lock(&prot_box->n_messages_lock);    // locks
    prot_box->n_messages++;                    // changes var
    mutex_unlock(&prot_box->n_messages_lock);  // unlocks

    //* Signals that a new message arrived
    pthread_cond_broadcast(&prot_box->box_cond_var);

    len = strlen(prot_message->message) + 1;  // Includes final '\0'

    // Writes the received message to the message box
    if ((n = tfs_write(prot_box_fd, prot_message->message, len)) > 0) {
      // Increments box size
      prot_box->box_size += (size_t)n;
    } else {
      // leaves loop -> publisher left
      break;
    }

    memset(prot_message->message, 0, sizeof(prot_message->message));
  }

  prot_box->n_publishers--;

  close(prot_session_fd);

  free(prot_message);

  tfs_close(prot_box_fd);

  return 0;
}

/**
 * @brief Create a subscriber that search by the right message in the box, open
 * the file and allocate memory and send the messages to this sub. After that,
 * while the sub keep continue to receive messages, it write them.
 *
 * @param pipe_name users pipe
 * @param prot_box_name message box in use
 */
int register_sub(const char *pipe_name, const char *prot_box_name) {
  int prot_box_fd = tfs_open(prot_box_name, 0);
  if (prot_box_fd < 0) {
    fprintf(stderr, "Err: couldn't open message prot_box");
    return -1;
  }

  int prot_session_fd;
  if ((prot_session_fd = open(pipe_name, O_WRONLY)) < 0) {
    fprintf(stderr, "Err: couldn't open session fifo");
    return -1;
  }

  // Tries to get the message box
  Message_Box *prot_box = find_message_box(prot_box_name);
  ssize_t n;

  if (prot_box == NULL) {
    fprintf(stderr, "Err: couldn't open message prot_box");
    return -1;
  }

  // Gets the current number of messages of the box
  mutex_lock(&prot_box->n_messages_lock);
  int n_messages = prot_box->n_messages;
  mutex_unlock(&prot_box->n_messages_lock);

  // Increments box's number of subscribers
  prot_box->n_subscribers++;

  // Allocs memory to hold messages
  Message_Protocol *prot_message =
      (Message_Protocol *)malloc(sizeof(Message_Protocol));

  prot_message->code = 10;

  memset(prot_message->message, 0, 1024);

  //* Reads if there are any messages in the message box
  if (tfs_read(prot_box_fd, prot_message->message, 1024) > 0) {
    // If there were received any messages, send them to client session fifo
    write(prot_session_fd, prot_message, sizeof(Message_Protocol));

    // Resets the memory for new messages
    memset(prot_message->message, 0, 1024);
  }

  while (1) {
    // Prevents active waiting
    mutex_lock(&prot_box->n_messages_lock);
    while (n_messages == prot_box->n_messages) {
      // Cycle to wait until cond var changes. If number of read messages is
      // equal to the number of messages in the box, keeps waiting.
      pthread_cond_wait(&prot_box->box_cond_var, &prot_box->n_messages_lock);
    }
    mutex_unlock(&prot_box->n_messages_lock);

    //* Reads a new message from the message box
    n = tfs_read(prot_box_fd, prot_message->message, 1024);
    if (n < 0) {
      // This occurs when the message box closes
      // TODO: Make this close when manager removes box
      // ? Change the tfs_unlink function to close the box?
      break;
    }
    n_messages++;
    if (n == 0) continue;
    if (write(prot_session_fd, prot_message, sizeof(Message_Protocol)) <= 0) {
      //* If write is < 0, than the pipe is now closed
      break;
    }

    // Resets the memory for new messages
    memset(prot_message->message, 0, 1024);
  }

  free(prot_message);

  tfs_close(prot_box_fd);
  close(prot_session_fd);

  return 0;
}

/**
 * @brief Create a box message where we allocate space for the response and if
 * the message box doesn't exist.
 *
 * @param pipe_name users pipe
 * @param prot_box_name message box in use
 */
int create_box(const char *pipe_name, const char *prot_box_name) {
  // Allocs memory for the response
  Box_Protocol *prot_response = (Box_Protocol *)malloc(sizeof(Box_Protocol));
  prot_response->code = 4;

  // Checks if box exists
  if (find_message_box(prot_box_name) != NULL) {
    prot_response->response = -1;
    strcpy(prot_response->error_message, "Message prot_box already exists");
  } else {
    int prot_box_fd;
    // TFS
    if ((prot_box_fd = tfs_open(prot_box_name, TFS_O_CREAT)) < 0) {
      prot_response->response = -1;
      strcpy(prot_response->error_message, "Error while creating prot_box");
    } else {
      prot_response->response = 0;
      add_box(prot_box_name, prot_box_fd);
      memset(prot_response->error_message, 0,
             strlen(prot_response->error_message));
    }
  }

  //
  int prot_session_fd;
  prot_session_fd = open(pipe_name, O_WRONLY);
  if (prot_session_fd < 0) {
    fprintf(stderr, "Err: couldn't open session fifo");
    return -1;
  }

  __uint8_t code = 4;

  if (write(prot_session_fd, &code, 1) < 0) {
    free(prot_response);
    fprintf(stderr, "Error while writing in manager fifo");
    return -1;
  }

  if (write(prot_session_fd, prot_response, sizeof(Box_Protocol)) < 0) {
    free(prot_response);
    fprintf(stderr, "Error while writing in manager fifo");
    return -1;
  }

  close(prot_session_fd);
  free(prot_response);
  return 0;
}

/**
 * @brief destroy a message box given.
 *
 * @param pipe_name users pipe
 * @param prot_box_name message box in use
 */
int destroy_box(const char *pipe_name, const char *prot_box_name) {
  Box_Protocol *prot_response = (Box_Protocol *)malloc(sizeof(Box_Protocol));

  prot_response->code = 6;

  if (tfs_unlink(prot_box_name) == -1) {
    prot_response->response = -1;
    strcpy(prot_response->error_message, "Error while deleting prot_box_name");
  } else {
    prot_response->response = 0;
    memset(prot_response->error_message, 0,
           strlen(prot_response->error_message));
  }

  int prot_session_fd;
  prot_session_fd = open(pipe_name, O_WRONLY);
  if (prot_session_fd < 0) {
    fprintf(stderr, "Err: couldn't open session fifo");
    return -1;
  }

  if (write(prot_session_fd, prot_response, sizeof(Box_Protocol)) < -1) {
    free(prot_response);
    fprintf(stderr, "Error while writing in manager fifo");
    return -1;
  }

  remove_box(prot_box_name);

  close(prot_session_fd);

  free(prot_response);

  return 0;
}

/**
 * @brief Send the box messages list.
 *
 * @param pipe_name users pipe
 */
int send_list_boxes_protocol(const char *pipe_name) {
  int prot_session_fd = open(pipe_name, O_WRONLY);
  if (prot_session_fd < 0) {
    fprintf(stderr, "Error while opening Manager Fifo in list_prot_boxes");
    return -1;
  }

  send_list_boxes(prot_session_fd);

  close(prot_session_fd);

  return 0;
}
