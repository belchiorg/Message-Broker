#include "logging.h"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  if (argc != 4) {
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
  }

  const char *register_pipe_name = argv[1];  // Nome do pipe do Cliente
  const char *pipe_name = argv[2];  // Canal que recebe as mensagens (servidor)
  const char *box_name = argv[3];   // Pipe de mensagem (Ficheiro TFS)

  WARN("unimplemented");  // TODO: implement
  return -1;
}
