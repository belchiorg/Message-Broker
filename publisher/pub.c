#include <fcntl.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../mbroker/protocol.h"
#include "../utils/logging.h"
#include "../utils/utils.h"

#define MAX_MESSAGE_LEN 292

const char *register_pipe_name;
Message_Protocol *message = NULL;
Registry_Protocol *registry = NULL;
int fd = -1;
int session = -1;

/**
 * @brief Free the message and the registry
 * @param sig signal received
 */
void sig_handler(int sig) {
  (void)sig;
  close(0);
}

/**
 * @brief Publisher will write the messages from stdin
 * @param argc max arguments that mbroker input can receive
 * @param argv contains the pipe name and the number of max sessions insert by
 * the user
 */
int main(int argc, char **argv) {
  // Check if theres any type of signal
  if (signal(SIGINT, sig_handler) == SIG_ERR) {
  }
  if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
  }
  if (signal(SIGPIPE, sig_handler) == SIG_ERR) {
  }

  if (argc != 4) {
    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
  }

  register_pipe_name = argv[1];     // Nome do pipe da sessão
  const char *pipe_name = argv[2];  // Canal que recebe as mensagens (servidor)
  const char *boxName = argv[3];    // Pipe de mensagem (Ficheiro TFS)

  // Open the file from the pipe
  fd = open(pipe_name, O_WRONLY | O_APPEND);
  if (fd < 0) {
    fprintf(stderr, "Error while opening fifo at publisher");
    raise(SIGTERM);
  }

  registry = (Registry_Protocol *)malloc(sizeof(Registry_Protocol));

  // Update the registry with the pipe and box message info
  registry->code = 1;
  strcpy(registry->register_pipe_name, register_pipe_name);
  strcat(registry->box_name, "/");
  strncat(registry->box_name, boxName, 31);

  // Create the fifo for the pipe
  if (mkfifo(register_pipe_name, 0777) < 0) {
    free(registry);
    fprintf(stderr, "Error while creating fifo");
    raise(SIGTERM);
  }

  // Write the registry in the file
  if (write(fd, registry, sizeof(Registry_Protocol)) < 0) {
    free(registry);
    fprintf(stderr, "Error while writing in fifo");
    raise(SIGTERM);
  }

  free(registry);
  registry = NULL;

  close(fd);

  session = open(register_pipe_name, O_WRONLY);
  if (session < 0) {
    fprintf(stderr, "Couldn't open session fifo");
    raise(SIGTERM);
  }

  ssize_t t = 0;

  // Allocate for the publisher message
  message = (Message_Protocol *)malloc(sizeof(Message_Protocol));
  message->code = 9;
  while (1) {
    memset(message->message, 0, 1024);

    // Receive from the stdin
    if (fgets(message->message, 1024, stdin) == NULL) break;

    t = (ssize_t)strlen(message->message);
    message->message[t - 1] = '\0';  // removes '\n'

    // Case that we have nothing more to write
    if (write(session, message, sizeof(Message_Protocol)) < 0) {
      free(message);
      fprintf(stderr, "Error while writing from stdin");
      raise(SIGTERM);
    }
  }

  free(message);

  unlink(register_pipe_name);

  return 0;
}
