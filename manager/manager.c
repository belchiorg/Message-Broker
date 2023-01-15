#include <signal.h>
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

Message_Box *box = NULL;
char *reg_pipe;
Registry_Protocol *registry = NULL;
Box_Protocol *response = NULL;
int session_fd = -1;
int server_fd = -1;

void sig_handler(int sig) {
  (void)sig;
  if (response != NULL) {
    free(response);
  }
  if (registry != NULL) {
    free(registry);
  }
  if (box != NULL) {
    free(box);
  }

  unlink(reg_pipe);
  close(session_fd);
  close(server_fd);

  exit(EXIT_SUCCESS);
}

void list_boxes() {
  session_fd = open(reg_pipe, O_RDONLY);
  if (session_fd < 0) {
    perror("Error while opening fifo at manager");
    exit(EXIT_FAILURE);
  }

  box = (Message_Box *)malloc(sizeof(Message_Box));

  ssize_t n = 0;

  ssize_t t;

  while (1) {
    t = read(session_fd, box, sizeof(Message_Box));

    if (t <= 0) break;

    n += t;
    fprintf(stdout, "%s %zu %zu %zu\n", box->box_name + 1, box->box_size,
            box->n_publishers, box->n_subscribers);

    memset(box, 0, sizeof(Message_Box));
  }
  free(box);

  if (n == 0) {
    fprintf(stdout, "NO BOXES FOUND\n");
  }

  unlink(reg_pipe);

  close(session_fd);
}

void create_delete_box() {
  session_fd = open(reg_pipe, O_RDONLY | O_APPEND);
  if (session_fd < 0) {
    perror("Error while opening fifo at manager");
    exit(EXIT_FAILURE);
  }

  response = (Box_Protocol *)malloc(sizeof(Box_Protocol));

  if (read(session_fd, response, sizeof(Box_Protocol)) < 0) {
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

  unlink(reg_pipe);

  close(session_fd);
}

int main(int argc, char **argv) {
  if (signal(SIGINT, sig_handler) == SIG_ERR) {
  }
  if (signal(SIGTERM, sig_handler) == SIG_ERR) {
  }
  if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
  }

  if (argc != 4 && argc != 5) {
    print_usage();
    exit(EXIT_FAILURE);
  }

  reg_pipe = argv[1];
  char *pipe_name = argv[2];
  char *action = argv[3];

  server_fd = open(pipe_name, O_WRONLY | O_APPEND);
  if (server_fd < 0) {
    fprintf(stderr, "Error while opening server fifo at manager");
    raise(SIGTERM);
  }

  registry = (Registry_Protocol *)malloc(sizeof(Registry_Protocol));

  if (argc == 4) {
    if (strcmp(action, "list") == 0) {
      registry->code = 7;
      strcpy(registry->register_pipe_name, reg_pipe);
    } else {
      print_usage();
      raise(SIGTERM);
    }
  } else {
    strcpy(registry->register_pipe_name, reg_pipe);
    strcat(registry->box_name, "/");
    strcat(registry->box_name, argv[4]);

    if (strcmp(action, "create") == 0) {
      registry->code = 3;
    } else if (strcmp(action, "remove") == 0) {
      registry->code = 5;
    } else {
      print_usage();
      raise(SIGTERM);
    }
  }

  if (mkfifo(reg_pipe, 0777)) {
    fprintf(stderr, "Error while making manager fifo");
    raise(SIGTERM);
  }

  if (write(server_fd, registry, sizeof(Registry_Protocol)) < 0) {
    fprintf(stderr, "Error while writing in fifo");
    raise(SIGTERM);
  }

  close(server_fd);

  if (registry->code == 7) {
    free(registry);
    list_boxes();
  } else {
    free(registry);
    create_delete_box();
  }

  unlink(reg_pipe);

  return 0;
}
