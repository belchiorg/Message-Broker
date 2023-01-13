#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../fs/operations.h"
#include "../producer-consumer/producer-consumer.h"
#include "../utils/logging.h"
#include "../utils/utils.h"
#include "message_box.h"
#include "protocol.h"

sem_t sessionsSem;
pc_queue_t queue;
Box_Node* box_list = NULL;

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

  pcq_create(&queue, (size_t)maxSessions);

  // if (sem_init(&sessionsSem, 0, maxSessions) == -1) {
  //   perror("sem_init");
  //   exit(EXIT_FAILURE);
  // }

  // sem_wait(&sessionsSem); //! Usar isto quando não houverem mais sessões

  tfs_init(NULL);

  box_list = NULL;

  unlink(pipeName);

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
    Registry_Protocol* registry =
        (Registry_Protocol*)malloc(sizeof(Registry_Protocol));
    // This loop reads the pipe, always expecting new messages

    if ((read(fd, registry, sizeof(Registry_Protocol))) != 0) {
      fprintf(stdout, "ERROR %d\n", registry->code);
      //* recebeu uma mensagem

      switch (registry->code) {
        case 1:
          register_pub(registry->register_pipe_name, registry->box_name,
                       box_list);
          break;

        case 2:
          register_sub(registry->register_pipe_name, registry->box_name,
                       box_list);
          break;

        case 3:
          create_box(registry->register_pipe_name, registry->box_name,
                     box_list);
          if (box_list == NULL) {
            puts("Nula");
          }
          break;

        case 4:
          //* resposta
          break;

        case 5:
          destroy_box(registry->register_pipe_name, registry->box_name,
                      box_list);
          break;

        case 6:
          //* resposta
          break;

        case 7:
          puts("hey");
          send_list_boxes(registry->register_pipe_name, box_list);
          break;

        case 8:
          //* resposta
          break;

        default:
          break;
      }
    }
    sleep(1);  //! Ta em espera ativa aqui uwu
    free(registry);
  }

  pcq_destroy(&queue);

  tfs_destroy();

  close(fd);

  return EXIT_FAILURE;
}
