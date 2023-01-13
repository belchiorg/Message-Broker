#include "message_box.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__uint8_t is_empty_list = 1;

__uint8_t check_if_empty_list() { return is_empty_list; }

void add_box(const char* box_name, Box_Node* box_list) {
  Message_Box* box = (Message_Box*)malloc(sizeof(Message_Box));

  box->code = 8;
  strcpy(box->box_name, box_name);
  box->box_size = 0;
  box->last = 0;
  box->n_publishers = 0;
  box->n_subscribers = 0;

  Box_Node* node = (Box_Node*)malloc(sizeof(Box_Node));
  node->next = NULL;
  node->box = box;
  Box_Node* ptr = box_list;

  if (check_if_empty_list() == 1 ||
      strcmp((ptr->box)->box_name, box->box_name) > 0) {
    puts("heya");
    box_list = node;
    if (box_list == NULL) puts("Nao sou null");
  } else {
    while ((ptr->next != NULL) &&
           (strcmp(((ptr->next)->box)->box_name, box->box_name) < 0)) {
      ptr = ptr->next;
    }
    if (ptr->next == NULL) {
      node->box->last = 1;
    } else {
      node->next = ptr->next;
      ptr->next = node;
    }
  }
  is_empty_list = 0;
}

void remove_box(const char* box_name, Box_Node* box_list) {
  if (box_list == NULL) {
    return;
  }
  if (strcmp(box_list->box->box_name, box_name) == 0) {
    Box_Node* ptr = box_list;
    free(ptr->box);
    free(ptr);
    box_list = box_list->next;
    if (box_list == NULL) is_empty_list = 1;
  } else {
    Box_Node* temp;
    Box_Node* current = box_list;
    while (box_list->next != NULL) {
      if (strcmp(current->next->box->box_name, box_name) == 0) {
        temp = current->next;
        Box_Node* post = temp->next;
        current->next = post;
        free(temp);
        break;
      } else {
        current = current->next;
      }
    }
  }
}
Message_Box* find_message_box(const char* box_name, Box_Node* box_list) {
  if (box_list == NULL) {
    return NULL;
  }
  if (strcmp(box_list->box->box_name, box_name) == 0) {
    return box_list->box;
  } else {
    Box_Node* next_box = box_list->next;
    while (next_box != NULL) {
      next_box = next_box->next;
      if (strcmp(next_box->box->box_name, box_name) == 0) {
        return next_box->box;
      }
    }
  }
  return NULL;
}
