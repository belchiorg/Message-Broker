#include <string.h>

#include "logging.h"

static void print_usage() {
  fprintf(stderr,
          "usage: \n"
          "   manager <register_pipe_name> create <box_name>\n"
          "   manager <register_pipe_name> remove <box_name>\n"
          "   manager <register_pipe_name> list\n");
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  if (argc != 4 || argc != 3) {
    print_usage();
    exit(EXIT_FAILURE);
  }

  char *reg_pipe = argv[1];
  char *action = argv[2];

  (void)reg_pipe;

  // TODO: verify if reg_pipe is valid :D

  if (argc == 3) {
    if (strcmp(action, "list") == 0) {
      WARN("unimplemented");  // TODO: implement
    } else {
      print_usage();
      exit(EXIT_FAILURE);
    }
  } else {
    if (strcmp(action, "create") == 0) {
      WARN("unimplemented");  // TODO: implement
    } else if (strcmp(action, "remove") == 0) {
      WARN("unimplemented");  // TODO: implement
    } else {
      print_usage();
      exit(EXIT_FAILURE);
    }
  }

  return 0;
}
