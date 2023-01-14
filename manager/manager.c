#include <string.h>

#include "../mbroker/message_box.h"
#include "../mbroker/protocol.h"
#include "../utils/logging.h"

static void print_usage() {
  fprintf(stderr,
          "usage: \n"
          "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
          "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
          "   manager <register_pipe_name> <pipe_name> list\n");
}

void list_boxes(const char *pipe_name) {
  int fd = open(pipe_name, O_RDONLY);
  if (fd < 0) {
    perror("Error while opening fifo at manager");
    exit(EXIT_FAILURE);
  }

  Message_Box *box = (Message_Box *)malloc(sizeof(Message_Box));

  ssize_t n = 0;

  ssize_t t;

  while (1) {
    t = read(fd, box, sizeof(Message_Box));

    if (t <= 0)
      break;

    n += t;
    fprintf(stdout, "%s %zu %zu %zu\n", box->box_name + 1, box->box_size,
            box->n_publishers, box->n_subscribers);

    memset(box, 0, sizeof(Message_Box));
  }
  free(box);

  if (n == 0) {
    fprintf(stdout, "NO BOXES FOUND\n");
  }

  unlink(pipe_name);

  close(fd);
}

void create_delete_box(const char *pipe_name) {
  int fd = open(pipe_name, O_RDONLY | O_APPEND);
  if (fd < 0) {
    perror("Error while opening fifo at manager");
    exit(EXIT_FAILURE);
  }

  Box_Protocol *response = (Box_Protocol *)malloc(sizeof(Box_Protocol));

  if (read(fd, response, sizeof(Box_Protocol)) < 0) {
    free(response);
    perror("Error while reading manager fifo");
    exit(EXIT_FAILURE);
  }

  if (response->response == 0) {
    fprintf(stdout, "OK\n");
  } else {
    fprintf(stdout, "ERROR %s\n", response->error_message);
  }

  free(response);

  unlink(pipe_name);

  close(fd);
}

int main(int argc, char **argv) {
  if (argc != 4 && argc != 5) {
    print_usage();
    exit(EXIT_FAILURE);
  }

  char *reg_pipe = argv[1];
  char *pipe_name = argv[2];
  char *action = argv[3];

  int fd = open(pipe_name, O_WRONLY | O_APPEND);
  if (fd < 0) {
    perror("Error while opening fifo at manager");
    exit(EXIT_FAILURE);
  }

  int to_list_question_mark = 1;

  // TODO: verify if reg_pipe is valid :D

  Registry_Protocol *registry =
      (Registry_Protocol *)malloc(sizeof(__uint8_t) + 256 + 32);

  if (argc == 4) {
    if (strcmp(action, "list") == 0) {
      registry->code = 7;
      strcpy(registry->register_pipe_name, reg_pipe);
    } else {
      free(registry);
      print_usage();
      exit(EXIT_FAILURE);
    }
  } else {
    to_list_question_mark = 0;
    char *box_name = argv[4];
    if (strcmp(action, "create") == 0) {
      registry->code = 3;
      strcpy(registry->register_pipe_name, reg_pipe);
      strcat(registry->box_name, "/");
      strcat(registry->box_name, box_name);
    } else if (strcmp(action, "remove") == 0) {
      registry->code = 5;
      strcpy(registry->register_pipe_name, reg_pipe);
      strcat(registry->box_name, "/");
      strcat(registry->box_name, box_name);
    } else {
      free(registry);
      print_usage();
      exit(EXIT_FAILURE);
    }
  }

  unlink(reg_pipe); //! -> mover poh final

  if (mkfifo(reg_pipe, 0777)) {
    free(registry);
    perror("Error while making manager fifo");
    exit(EXIT_FAILURE);
  }

  if (write(fd, registry, sizeof(Registry_Protocol)) < 0) {
    free(registry);
    perror("Error while writing in fifo");
    exit(EXIT_FAILURE);
  }

  free(registry);

  close(fd);

  if (to_list_question_mark) {
    list_boxes(reg_pipe);
  } else {
    create_delete_box(reg_pipe);
  }

  return 0;
}
