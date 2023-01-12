#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../fs/operations.h"
#include "../producer-consumer/producer-consumer.h"
#include "../registry/registry.h"
#include "../utils/logging.h"
#include "../utils/utils.h"

sem_t sessionsSem;
pc_queue_t queue;

int main(int argc, char** argv) {
  // expected argv:
  // 0 - nome do programa
  // 1 - pipename
  // 2 - max sessions

  if (argc != 3) {
    fprintf(stderr, "usage: mbroker <pipeName> <maxSessions>\n");
    exit(EXIT_FAILURE);
  }

  const char* pipeName = argv[1];
  const size_t maxSessions = (size_t)atoi(argv[2]);
  char buf[MAX_BLOCK_LEN];

  pcq_create(&queue, (size_t)maxSessions);

  // if (sem_init(&sessionsSem, 0, maxSessions) == -1) {
  //   perror("sem_init");
  //   exit(EXIT_FAILURE);
  // }

  // sem_wait(&sessionsSem); //! Usar isto quando não houverem mais sessões

  tfs_init(NULL);

  if (unlink(pipeName) < 0) {
    tfs_destroy();
    exit(EXIT_FAILURE);
  }

  if (mkfifo(pipeName, 0640) < 0) {
    tfs_destroy();
    exit(EXIT_FAILURE);
  }

  int fd = open(pipeName, TFS_O_TRUNC);
  if (fd < 0) {
    tfs_destroy();
    exit(EXIT_FAILURE);
  }

  while (1) {
    // This loop reads the pipe, always expecting new messages

    if ((read(fd, buf, MAX_FILE_NAME)) != 0) {
      //* recebeu uma mensagem
      Protocol* registry = (Protocol*)malloc(sizeof(__uint8_t) + 256 + 32);

      switch (registry->code) {
        case 1:
          registerPub(registry->register_pipe_name, registry->box_name);
          break;

        case 2:
          registerSub(registry->register_pipe_name, registry->box_name);
          break;

        case 3:
          createBox(registry->register_pipe_name, registry->box_name);
          break;

        case 4:
          //* resposta
          break;

        case 5:
          destroyBox(registry->register_pipe_name, registry->box_name);
          break;

        case 6:
          //* resposta
          break;

        case 7:
          listBoxes();
          break;

        case 8:
          //* resposta
          break;

        default:
          break;
      }
      free(registry);
    }
    sleep(1);  //! Ta em espera ativa aqui uwu
  }

  pcq_destroy(&queue);

  tfs_destroy();

  close(fd);

  return EXIT_FAILURE;
}
