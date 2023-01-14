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

void* workerThreadsFunc(void* arg) {
  while (1) {
    Registry_Protocol* registry =
        (Registry_Protocol*)malloc(sizeof(Registry_Protocol));
    // This loop reads the pipe, always expecting new registrys

    if ((read(fd, registry, sizeof(Registry_Protocol))) != 0) {
      // Received a registry

      switch (registry->code) {
        case 1:
          register_pub(registry->register_pipe_name, registry->box_name);
          break;

        case 2:
          register_sub(registry->register_pipe_name, registry->box_name);
          break;

        case 3:
          create_box(registry->register_pipe_name, registry->box_name);
          break;

        case 5:
          destroy_box(registry->register_pipe_name, registry->box_name);
          break;

        case 7:
          send_list_boxes(registry->register_pipe_name);
          break;

        default:
          break;
      }
    }
  }
}

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

  pthread_t workerThreads[maxSessions];
  for (int i = 0; i < maxSessions; i++) {
    pthread_create(workerThreads[i], NULL, &workerThreadFunc, NULL);
  }

  pcq_create(&queue, (size_t)maxSessions);

  if (sem_init(&sessionsSem, 0, (unsigned int)maxSessions) == -1) {
    perror("sem_init");
    exit(EXIT_FAILURE);
  }

  tfs_init(NULL);

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
    // This loop reads the pipe, always expecting new registrys

    if ((read(fd, registry, sizeof(Registry_Protocol))) != 0) {
      // Received a registry

      switch (registry->code) {
        case 1:
          register_pub(registry->register_pipe_name, registry->box_name);
          break;

        case 2:
          register_sub(registry->register_pipe_name, registry->box_name);
          break;

        case 3:
          create_box(registry->register_pipe_name, registry->box_name);
          break;

        case 5:
          destroy_box(registry->register_pipe_name, registry->box_name);
          break;

        case 7:
          send_list_boxes(registry->register_pipe_name);
          break;

        default:
          break;
      }
    }
    free(registry);
  }

  pcq_destroy(&queue);

  tfs_destroy();

  close(fd);

  sem_destroy(&sessionsSem);

  return EXIT_FAILURE;
}
