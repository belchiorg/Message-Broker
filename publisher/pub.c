#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../mbroker/protocol.h"
#include "../utils/logging.h"
#include "../utils/utils.h"

#define MAX_MESSAGE_LEN 292

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  if (argc != 4) {
    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
  }

  const char *register_pipe_name = argv[1];  // Nome do pipe do Cliente
  const char *pipe_name = argv[2];  // Canal que recebe as mensagens (servidor)
  const char *boxName = argv[3];    // Pipe de mensagem (Ficheiro TFS)

  int fd = open(pipe_name, O_WRONLY | O_APPEND);
  if (fd < 0) {
    perror("Error while opening fifo at publisher");
    exit(EXIT_FAILURE);
  }

  Registry_Protocol *registry =
      (Registry_Protocol *)malloc(sizeof(Registry_Protocol));

  registry->code = 1;
  strcpy(registry->register_pipe_name, register_pipe_name);
  strcat(registry->box_name, "/");
  strncat(registry->box_name, boxName, 31);

  unlink(register_pipe_name);  //!-> passou para o publisher

  if (write(fd, registry, sizeof(Registry_Protocol)) < 0) {
    free(registry);
    perror("Error while writing in fifo");
    exit(EXIT_FAILURE);
  }

  if (mkfifo(register_pipe_name, 0777) < 0) {
    perror("Error while creating fifo");
    exit(EXIT_FAILURE);
  }

  free(registry);

  close(fd);

  int session = open(register_pipe_name, O_WRONLY);
  if (session < 0) {
    perror("Couldn't open session fifo");
    exit(EXIT_FAILURE);
  }

  Message_Protocol *message;
  ssize_t n = 0;
  ssize_t t = 0;

  message = (Message_Protocol *)malloc(sizeof(Message_Protocol));
  message->code = 9;
  while (1) {
    memset(message->message, 0, 1024);
    t = 0;

    if (fgets(message->message, 1024, stdin) == NULL) break;

    if (message->message[0] == '*') {
      break;
    }

    t = (ssize_t)strlen(message->message);
    message->message[t - 1] = '\0';  // removes '\n'

    n += t;

    if (write(session, message, sizeof(Message_Protocol)) < 0) {
      free(message);
      perror("Error while writing from stdin");
      exit(EXIT_FAILURE);
    }
  }

  free(message);

  close(session);

  unlink(register_pipe_name);

  return 0;
}
