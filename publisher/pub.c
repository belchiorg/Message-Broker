#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../registry/registry.h"
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

  char reg[MAX_MESSAGE_LEN];

  Protocol *registry = (Protocol *)malloc(256 + 33 + sizeof(__uint8_t));

  registry->code = 1;
  strncpy(registry->register_pipe_name, register_pipe_name, 256);
  strncat(registry->box_name, "/", 1);
  strncat(registry->box_name, boxName, 31);

  if (write(fd, registry, MAX_MESSAGE_LEN) < 0) {
    free(registry);
    perror("Error while writing in fifo");
    exit(EXIT_FAILURE);
  }

  free(registry);

  close(fd);
  sleep(2);  //! Yau <- tirar isto

  int session = open(register_pipe_name, O_WRONLY);
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

  unlink(register_pipe_name);

  return 0;
}
