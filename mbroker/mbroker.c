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

void* workerThreadsFunc() {
  while (1) {
    printf("%lu\n", queue.pcq_current_size);
    Registry_Protocol* registry = pcq_dequeue(&queue);
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
    free(registry);
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

  pcq_create(&queue, (size_t)maxSessions);

  if (sem_init(&sessionsSem, 0, (unsigned int)maxSessions) == -1) {
    perror("sem_init");
    exit(EXIT_FAILURE);
  }

  tfs_init(NULL);

  pthread_t workerThreads[maxSessions];
  for (int i = 0; i < maxSessions; i++) {
    pthread_create(&workerThreads[i], NULL, &workerThreadsFunc, NULL);
  }

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

  Registry_Protocol* registry;
  while (1) {
    registry = (Registry_Protocol*)malloc(sizeof(Registry_Protocol));
    if ((read(fd, registry, sizeof(Registry_Protocol))) != 0) {
      // Received a registry
      pcq_enqueue(&queue, registry);
    }
  }

  pcq_destroy(&queue);

  tfs_destroy();

  close(fd);

  sem_destroy(&sessionsSem);

  return EXIT_FAILURE;
}
