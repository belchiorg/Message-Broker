#include <string.h>

#include "../mbroker/protocol.h"
#include "../utils/logging.h"

static void print_usage() {
  fprintf(stderr,
          "usage: \n"
          "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
          "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
          "   manager <register_pipe_name> <pipe_name> list\n");
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

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

  unlink(reg_pipe);
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

  fd = open(reg_pipe, O_RDONLY | O_APPEND);
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

  free(response);

  close(fd);

  return 0;
}
