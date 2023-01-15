#include <fcntl.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../mbroker/protocol.h"
#include "../utils/utils.h"
#include "logging.h"

int messages_n = 0;
int fd;
int session;
const char *register_pipe_name;
Registry_Protocol *registry = NULL;

Message_Protocol *message = NULL;

atomic_int end_program = 0;

/**
 * @brief Free the message and the registry
 * @param sig signal received
 */
void sig_handler(int sig) {
  (void)sig;
  end_program++;
}

/**
 * @brief
 * @param sig signal received
 */
int main(int argc, char **argv) {
  // Check if theres any type of signal
  if (signal(SIGINT, sig_handler) == SIG_ERR) {
  }
  if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
  }
  if (signal(SIGPIPE, sig_handler) == SIG_ERR) {
  }
  if (signal(SIGTERM, sig_handler) == SIG_ERR) {
  }

  if (argc != 4) {
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
  }

  register_pipe_name = argv[1];     // Nome do pipe do Cliente
  const char *pipe_name = argv[2];  // Canal que recebe as mensagens (servidor)
  const char *box_name = argv[3];   // Pipe de mensagem (Ficheiro TFS)

  // Open the file from the pipe
  fd = open(pipe_name, O_WRONLY | O_APPEND);

  if (fd < 0) {
    fprintf(stderr, "Error while opening fifo at publisher");
    raise(SIGTERM);
  }

  if (mkfifo(register_pipe_name, 0640) < 0) {
    fprintf(stderr, "Error while opening fifo at publisher");
    raise(SIGTERM);
  }

  registry = (Registry_Protocol *)malloc(sizeof(Registry_Protocol));

  // Update the registry with the pipe and box message info
  registry->code = 2;
  strcpy(registry->register_pipe_name, register_pipe_name);
  strcat(registry->box_name, "/");
  strncat(registry->box_name, box_name, 31);

  // Write the registry in the file
  if (write(fd, registry, sizeof(Registry_Protocol)) < 0) {
    close(fd);
    fprintf(stderr, "Error while writing in fifo");
    raise(SIGTERM);
  }

  free(registry);
  registry = NULL;

  close(fd);

  // Open the pipe session
  if ((session = open(register_pipe_name, O_RDONLY)) < 0) {
    fprintf(stderr, "Couldn't open session fifo");
    raise(SIGTERM);
  }

  // Allocate for the subscriber receive message
  message = (Message_Protocol *)malloc(sizeof(Message_Protocol));

  ssize_t n;

  // Read all the message received, increments the number of messages and
  // increment again by the number of lines the message have.
  // After read the message print it in stdout
  if (!end_program &&
      ((n = read(session, message, sizeof(Message_Protocol))) > 0)) {
    messages_n++;
    for (int i = 0; (i < 1023 && !(message->message[i] == '\0' &&
                                   message->message[i + 1] == '\0'));
         i++) {
      if (message->message[i] == '\0') {
        message->message[i] = '\n';
        messages_n++;
      }
    }
    fprintf(stdout, "%s\n", message->message);

    memset(message->message, 0, sizeof(Message_Protocol));
  }

  while (!end_program &&
         ((n = read(session, message, sizeof(Message_Protocol))) > 0)) {
    fprintf(stdout, "%s\n", message->message);
    messages_n++;
    memset(message->message, 0, sizeof(Message_Protocol));
  }

  fprintf(stdout, "%d\n", messages_n);

  free(message);
  unlink(register_pipe_name);
  close(session);
  exit(EXIT_SUCCESS);

  return 0;
}
