#ifndef __BOX_H__
#define __BOX_H__

#include <pthread.h>
#include <sys/types.h>

typedef struct Message_Box {
  __uint8_t code;
  __uint8_t last;
  char box_name[32];
  __uint64_t box_size;
  __uint64_t n_publishers;
  __uint64_t n_subscribers;
  int box_fd;
  pthread_cond_t box_cond_var;
  pthread_mutex_t n_messages_lock;
  int n_messages;

} Message_Box;

typedef struct Box_Node {
  Message_Box *box;
  struct Box_Node *next;
} Box_Node;

void add_box(const char *box_name, int box_fd);

int remove_box(const char *box_name);

Message_Box *find_message_box(const char *box_name);

__uint8_t check_if_empty_list();

void send_list_boxes_from_other_file(int pipe_fd);

void destroy_all_boxes();

#endif