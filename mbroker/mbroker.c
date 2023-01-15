#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
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

size_t max_sessions_var = 0;
pc_queue_t queue;
int fd;
const char *pipe_name;
pthread_t *worker_threadsPtr;

void sig_handler(int sig) {
  unlink(pipe_name);

  for (int i = 0; i < max_sessions_var; i++) {
    pthread_join(worker_threadsPtr[i], NULL);
  }

  destroy_all_boxes();

  pcq_destroy(&queue);

  tfs_destroy();

  close(fd);

  if (sig == SIGTERM) {
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

void *worker_threads_func() {
  while (1) {
    Registry_Protocol *registry = pcq_dequeue(&queue);
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

int main(int argc, char **argv) {
  // expected argv:
  // 0 - nome do programa
  // 1 - pipe_name
  // 2 - max sessions

  if (signal(SIGINT, sig_handler) == SIG_ERR) {
  }
  if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
  }
  if (signal(SIGTERM, sig_handler) == SIG_ERR) {
  }
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
  }

  if (argc != 3) {
    fprintf(stderr, "usage: mbroker <pipe_name> <max_sessions>\n");
    exit(EXIT_FAILURE);
  }

  pipe_name = argv[1];
  const size_t max_sessions = (size_t)atoi(argv[2]);

  max_sessions_var = max_sessions;

  pcq_create(&queue, (size_t)max_sessions);

  tfs_init(NULL);

  pthread_t worker_threads[max_sessions];
  worker_threadsPtr = worker_threads;
  for (int i = 0; i < max_sessions; i++) {
    pthread_create(&worker_threads[i], NULL, &worker_threads_func, NULL);
  }

  if (mkfifo(pipe_name, 0640) < 0) {
    fprintf(stderr, "Error while creating server fifo");
    raise(SIGTERM);
  }

  fd = open(pipe_name, TFS_O_TRUNC);
  if (fd < 0) {
    fprintf(stderr, "Error while opening server fifo");
    raise(SIGTERM);
  }

  Registry_Protocol *registry;
  while (1) {
    registry = (Registry_Protocol *)malloc(sizeof(Registry_Protocol));
    if ((read(fd, registry, sizeof(Registry_Protocol))) != 0) {
      // Received a registry
      pcq_enqueue(&queue, registry);
    }
  }

  return EXIT_FAILURE;
}
