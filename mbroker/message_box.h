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

/**
 * @brief Adds a new box protocol to the global list. The box is inserted in the
 * correct position (list is sorted)
 *
 * @param box_name Message box name and directory in TFS
 * @param box_fd Message box file descriptor in TFS
 */
void add_box(const char *box_name, int box_fd);

/**
 * @brief  Removes box from list and frees it.
 *
 * @param box_name
 * @return 0 if it was sucessfull, -1 if it box didn't exist
 */
int remove_box(const char *box_name);

/**
 * @brief  Finds the message box with given box_name
 *
 * @param box_name  message box name and directory in TFS
 * @return the message box. If no box was found, return NULL
 */
Message_Box *find_message_box(const char *box_name);

/**
 * @brief Checks if the list is empty
 *
 * @return 1 if list is empty, 0 if not
 */
__uint8_t check_if_empty_list();

/**
 * @brief Function that sends box protocols to a given session file descriptor
 *
 * @param pipe_fd session file descriptor of the manager pipe.
 */
void send_list_boxes(int pipe_fd);

/**
 * @brief Function that deletes every message box of the list
 *
 */
void destroy_all_boxes();

#endif