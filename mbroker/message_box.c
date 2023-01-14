#include "message_box.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../fs/operations.h"

__uint8_t is_empty_list = 1;

Box_Node *box_list = NULL;

__uint8_t check_if_empty_list() { return is_empty_list; }

void add_box(const char *box_name, int box_fd) {
  Message_Box *box = (Message_Box *)malloc(sizeof(Message_Box));

  box->code = 8;
  strcpy(box->box_name, box_name);
  box->box_size = 0;
  box->last = 0;
  box->n_publishers = 0;
  box->n_subscribers = 0;
  box->box_fd = box_fd;
  pthread_cond_init(&box->box_cond_var, 0);

  Box_Node *node = (Box_Node *)malloc(sizeof(Box_Node));
  node->next = NULL;
  node->box = box;
  Box_Node *ptr = box_list;

  if (check_if_empty_list() == 1) {
    box_list = node;
  } else if (strcmp((ptr->box)->box_name, box->box_name) > 0) {
    node->next = box_list;
    box_list = node;
  } else {
    while ((ptr->next != NULL) &&
           (strcmp(((ptr->next)->box)->box_name, box->box_name) < 0)) {
      ptr = ptr->next;
    }
    if (ptr->next == NULL) {
      ptr->box->last = 0;
      node->box->last = 1;
      ptr->next = node;
    } else {
      node->next = ptr->next;
      ptr->next = node;
    }
  }
  is_empty_list = 0;
}

int remove_box(const char *box_name) {
  if (box_list == NULL) {
    return 0;
  }
  if (strcmp(box_list->box->box_name, box_name) == 0) {
    Box_Node *ptr = box_list;
    box_list = box_list->next;
    pthread_cond_destroy(&ptr->box->box_cond_var);
    tfs_close(ptr->box->box_fd);
    free(ptr->box);
    free(ptr);
    if (box_list == NULL) is_empty_list = 1;
  } else {
    Box_Node *temp;
    Box_Node *current = box_list;
    while (current->next != NULL) {
      if (strcmp(current->next->box->box_name, box_name) == 0) {
        temp = current->next;
        Box_Node *next = temp->next;
        current->next = next;
        pthread_cond_destroy(&temp->box->box_cond_var);
        tfs_close(temp->box->box_fd);
        free(temp->box);
        free(temp);
        break;
      } else {
        current = current->next;
      }
    }
  }
  return 1;
}

Message_Box *find_message_box(const char *box_name) {
  if (box_list == NULL) {
    return NULL;
  }
  if (strcmp(box_list->box->box_name, box_name) == 0) {
    return box_list->box;
  } else {
    Box_Node *next_box = box_list->next;
    while (next_box != NULL) {
      if (strcmp(next_box->box->box_name, box_name) == 0) {
        return next_box->box;
      }
      next_box = next_box->next;
    }
  }
  return NULL;
}

void send_list_boxes_so_que_la_do_outro_ficheiro(int pipe_fd) {
  Box_Node *ptr = box_list;

  while (ptr != NULL) {
    Message_Box *box = ptr->box;
    fprintf(stdout, "%s %zu %zu %zu\n", box->box_name + 1, box->box_size,
            box->n_publishers, box->n_subscribers);
    if (write(pipe_fd, box, sizeof(Message_Box)) < 0) {
      perror("Error while writing in manager Fifo");
      exit(EXIT_FAILURE);
    }
    ptr = ptr->next;
  }
}
