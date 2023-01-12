#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../utils/logging.h"
#include "../utils/utils.h"

#define MAX_MESSAGE_LEN 292

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  if (argc != 4) {
    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
  }

  const char *registerPipeName = argv[1];  // Nome do pipe do Cliente
  const char *pipeName = argv[2];  // Canal que recebe as mensagens (servidor)
  const char *boxName = argv[3];   // Pipe de mensagem (Ficheiro TFS)

  int fd = open(pipeName, O_WRONLY | O_APPEND);
  if (fd < 0) {
    perror("Error while opening fifo at publisher");
    exit(EXIT_FAILURE);
  }

  char reg[MAX_MESSAGE_LEN];

  memset(reg, 0, MAX_MESSAGE_LEN);

  strcat(reg, "1|");
  strncat(reg, registerPipeName, 256);
  strcat(reg, "|/");
  strncat(reg, boxName, 32);

  if (write(fd, reg, MAX_MESSAGE_LEN) < 0) {
    perror("Error while writing in fifo");
    exit(EXIT_FAILURE);
  }

  close(fd);
  sleep(2);

  int session = open(registerPipeName, O_WRONLY);
  if (session < 0) {
    perror("Couldn't open session fifo");
    exit(EXIT_FAILURE);
  }

  char buffer[MAX_BLOCK_LEN];

  while (1) {
    fscanf(stdin, "%s", buffer);
    if (buffer[2] == EOF) {
      break;
    }
    // buffer[MAX_BLOCK_LEN - 1] = 0;  // ? Limit the buffer

    write(session, buffer, MAX_BLOCK_LEN);
    break;

    memset(buffer, 0, MAX_BLOCK_LEN);
  }

  close(session);

  unlink(registerPipeName);

  return 0;
}
