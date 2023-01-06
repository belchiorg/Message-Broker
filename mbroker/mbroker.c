#include "logging.h"
#include "operations.h"
#include "state.h"
#include "unistd.h"

int main(int argc, char **argv) {
  // expected argv:
  // 0 - nome do programa
  // 1 - pipename
  // 2 - max sessions
  (void)argc;
  (void)argv;

  if (argc != 3) {
    fprintf(stderr, "usage: mbroker <pipename>\n");
    exit(EXIT_FAILURE);
  }

  char *pipename = argv[1];
  int max_sessions = atoi(argv[2]);
  char buf[MAX_MESSAGE_LEN];

  tfs_unlink(pipename);

  if (mkfifo(pipename, 0640) < 0) {
    exit(EXIT_FAILURE);
  }

  int fd = open(pipename, TFS_O_TRUNC);
  if (fd < 0) {
    exit(EXIT_FAILURE);
  }

  int n;
  while (1) {
    if (n = tfs_read(fd, buf, MAX_FILE_NAME - 1) <= 0) {
      break;
    }
  }

  close(fd);

  return EXIT_FAILURE;
}
