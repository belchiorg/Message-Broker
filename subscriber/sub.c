#include <fcntl.h>
#include <string.h>

#include "../utils/utils.h"
#include "logging.h"

#define MAX_MESSAGE_LEN 291

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  if (argc != 4) {
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
  }

  const char *registerPipeName = argv[1];  // Nome do pipe do Cliente
  const char *pipeName = argv[2];  // Canal que recebe as mensagens (servidor)
  const char *boxName = argv[3];   // Pipe de mensagem (Ficheiro TFS)

  int fd = open(pipeName, O_WRONLY | O_APPEND);
  if (fd < 0) {
    perror("Error while opening fifo at publisher");
    exit(EXIT_FAILURE);
  }

  char reg[MAX_MESSAGE_LEN] = "";

  strncat(reg, "2|", 2);
  strncat(reg, registerPipeName, 256);
  strncat(reg, "|", 1);
  strncat(reg, boxName, 32);

  if (write(fd, reg, MAX_MESSAGE_LEN) < 0) {
    close(fd);
    perror("Error while writing in fifo");
    exit(EXIT_FAILURE);
  }

  close(fd);

  int session;
  if (session = open(registerPipeName, O_WRONLY) < 0) {
    perror("Couldn't open session fifo");
    exit(EXIT_FAILURE);
  }

  char message[MESSAGE_SIZE];

  while (read(session, message, MESSAGE_SIZE) > 0) {
    // TODO: implement the cycle that reads new messages from the server
    fprintf(stdout, message);
  }

  unlink(registerPipeName);

  return 0;
}
