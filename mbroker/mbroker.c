#include "../utils/utils.h"
#include "consts.h"
#include "logging.h"
#include "operations.h"
#include "state.h"
#include "unistd.h"

int registerPub(const char* register_pipe, const char* pipename,
                const char* box_name) {}

int main(int argc, char** argv) {
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

  const char* pipeName = argv[1];
  const int maxSessions = atoi(argv[2]);
  char buf[MAX_BLOCK_LEN];

  tfs_unlink(pipeName);

  if (mkfifo(pipeName, 0640) < 0) {
    exit(EXIT_FAILURE);
  }

  int fd = open(pipeName, TFS_O_TRUNC);
  if (fd < 0) {
    exit(EXIT_FAILURE);
  }

  int n;
  while (1) {
    if (n = tfs_read(fd, buf, MAX_FILE_NAME - 1) == 0) {
      break;
    }
  }

  close(fd);

  return EXIT_FAILURE;
}
