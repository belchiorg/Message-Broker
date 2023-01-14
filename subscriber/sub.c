#include <fcntl.h>
#include <signal.h>
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

void catch_CTRLC(int sig) {
  (void)sig;
  if (message != NULL) {
    free(message);
  }
  if (registry != NULL) {
    free(registry);
  }
  unlink(register_pipe_name);
  close(fd);
  close(session);
  fprintf(stdout, "%d\n", messages_n);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  if (signal(SIGINT, catch_CTRLC) == SIG_ERR) {
  }
  if (signal(SIGQUIT, catch_CTRLC) == SIG_ERR) {
  }

  if (argc != 4) {
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
  }

  register_pipe_name = argv[1];     // Nome do pipe do Cliente
  const char *pipe_name = argv[2];  // Canal que recebe as mensagens (servidor)
  const char *box_name = argv[3];   // Pipe de mensagem (Ficheiro TFS)

  fd = open(pipe_name, O_WRONLY | O_APPEND);

  if (fd < 0) {
    perror("Error while opening fifo at publisher");
    exit(EXIT_FAILURE);
  }

  if (mkfifo(register_pipe_name, 0640) < 0) {
    perror("Error while creating fifo");
    exit(EXIT_FAILURE);
  }

  registry = (Registry_Protocol *)malloc(sizeof(Registry_Protocol));

  registry->code = 2;
  strcpy(registry->register_pipe_name, register_pipe_name);
  strcat(registry->box_name, "/");
  strncat(registry->box_name, box_name, 31);

  if (write(fd, registry, sizeof(Registry_Protocol)) < 0) {
    close(fd);
    perror("Error while writing in fifo");
    exit(EXIT_FAILURE);
  }

  free(registry);
  registry = NULL;

  close(fd);

  if ((session = open(register_pipe_name, O_RDONLY)) < 0) {
    perror("Couldn't open session fifo");
    exit(EXIT_FAILURE);
  }

  message = (Message_Protocol *)malloc(sizeof(Message_Protocol));

  ssize_t n;

  if ((n = read(session, message, sizeof(Message_Protocol))) > 0) {
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

  while ((n = read(session, message, sizeof(Message_Protocol))) >= 0) {
    fprintf(stdout, "%s\n", message->message);
    messages_n++;
    memset(message->message, 0, sizeof(Message_Protocol));
  }

  return 0;
}
