#include "message_box.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../fs/operations.h"

/**
 * @brief Linked List that contains all message boxes in the system
 *
 */
Box_Node *box_list = NULL;

/**
 * @brief Checks if the list is empty
 *
 * @return 1 if list is empty, 0 if not
 */
__uint8_t check_if_empty_list() { return box_list == NULL; }

/**
 * @brief Adds a new box protocol to the global list. The box is inserted in the
 * correct position (list is sorted)
 *
 * @param box_name Message box name and directory in TFS
 * @param box_fd Message box file descriptor in TFS
 */
void add_box(const char *box_name, int box_fd) {
    // Creates the box
    Message_Box *box = (Message_Box *)malloc(sizeof(Message_Box));

    // Initializes the box
    box->code = 8;
    strcpy(box->box_name, box_name);
    box->box_size = 0;
    box->last = 0;
    box->n_publishers = 0;
    box->n_subscribers = 0;
    box->box_fd = box_fd;
    pthread_cond_init(&box->box_cond_var, 0);

    // Creates the node to add it to the list
    Box_Node *node = (Box_Node *)malloc(sizeof(Box_Node));
    node->next = NULL;
    node->box = box;
    Box_Node *ptr = box_list;

    if (check_if_empty_list() == 1) {
        // If list is empty, insert node in head
        box_list = node;
    } else if (strcmp((ptr->box)->box_name, box->box_name) > 0) {
        // Change current head case
        node->next = box_list;
        box_list = node;
    } else {
        while ((ptr->next != NULL) &&
               (strcmp(((ptr->next)->box)->box_name, box->box_name) < 0)) {
            ptr = ptr->next;
        }
        // Looks for the correct position
        if (ptr->next == NULL) {
            // Insert in last position case
            ptr->box->last = 0;
            node->box->last = 1;
            ptr->next = node;
        } else {
            // Insert in the middle of the list case
            node->next = ptr->next;
            ptr->next = node;
        }
    }
}

/**
 * @brief  Removes box from list and frees it.
 *
 * @param box_name
 * @return 0 if it was sucessfull, -1 if it box didn't exist
 */
int remove_box(const char *box_name) {
    // Checks if list is empty
    if (box_list == NULL) {
        return -1;
    }

    if (strcmp(box_list->box->box_name, box_name) == 0) {
        // Remove head case

        Box_Node *ptr = box_list;
        box_list = box_list->next;
        pthread_cond_destroy(&ptr->box->box_cond_var);
        tfs_close(ptr->box->box_fd);
        free(ptr->box);
        free(ptr);
        return 0;

    } else {
        // Looks for the box in list

        Box_Node *temp;
        Box_Node *current = box_list;
        while (current->next != NULL) {
            // checks if the next box is the one to be deleted

            if (strcmp(current->next->box->box_name, box_name) == 0) {
                // Found the box to delete
                temp = current->next;
                Box_Node *next = temp->next;
                current->next = next;
                pthread_cond_destroy(&temp->box->box_cond_var);
                tfs_close(temp->box->box_fd);
                free(temp->box);
                free(temp);
                return 0;

            } else {
                // tries next box
                current = current->next;
            }
        }
    }
    return -1;
}

/**
 * @brief  Finds the message box with given box_name
 *
 * @param box_name  message box name and directory in TFS
 * @return the message box. If no box was found, return NULL
 */
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

/**
 * @brief Function that sends box protocols to a given session file descriptor
 *
 * @param pipe_fd session file descriptor of the manager pipe.
 */
void send_list_boxes(int pipe_fd) {
    Box_Node *ptr = box_list;

    while (ptr != NULL) {
        Message_Box *box = ptr->box;
        if (write(pipe_fd, box, sizeof(Message_Box)) < 0) {
            fprintf(stderr, "Error while writing in manager Fifo");
            return;
        }
        ptr = ptr->next;
    }
}

/**
 * @brief Function that deletes every message box of the list
 *
 */
void destroy_all_boxes() {
    if (box_list == NULL) {
        return;
    } else {
        Box_Node *ptr;
        while (box_list != NULL) {
            ptr = box_list;
            box_list = box_list->next;
            pthread_cond_destroy(&ptr->box->box_cond_var);
            tfs_unlink(ptr->box->box_name);
            tfs_close(ptr->box->box_fd);
            free(ptr->box);
            free(ptr);
        }
    }
}
