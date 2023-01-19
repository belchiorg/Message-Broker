#include <signal.h>
#include <string.h>

#include "../mbroker/message_box.h"
#include "../mbroker/protocol.h"
#include "../utils/logging.h"

/**
 * @brief Show the right way to insert the manager command.
 */
static void print_usage() {
    fprintf(stderr,
            "usage: \n"
            "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> list\n");
}

Message_Box *box = NULL;
char *reg_pipe;
Registry_Protocol *registry = NULL;
Box_Protocol *response = NULL;
int session_fd = -1;
int server_fd = -1;

/**
 * @brief Signal handler that unlinks the pipe, destroys all the box messages
 * and closes TFS.
 *
 * @param sig signal received
 */
void sig_handler(int sig) {
    (void)sig;
    if (response != NULL) {
        free(response);
    }
    if (registry != NULL) {
        free(registry);
    }
    if (box != NULL) {
        free(box);
    }

    unlink(reg_pipe);
    close(session_fd);
    close(server_fd);

    exit(EXIT_SUCCESS);
}

/**
 * @brief list all the boxe messages in the session
 *
 * @param sig signal received
 */
void list_boxes() {
    // Open the pipe sessions
    session_fd = open(reg_pipe, O_RDONLY);
    if (session_fd < 0) {
        perror("Error while opening fifo at manager");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the box message
    box = (Message_Box *)malloc(sizeof(Message_Box));

    ssize_t n = 0;

    ssize_t t;

    // read until don't find more boxes in the session
    while (1) {
        t = read(session_fd, box, sizeof(Message_Box));

        if (t <= 0)
            break;

        n += t;
        fprintf(stdout, "%s %zu %zu %zu\n", box->box_name + 1, box->box_size,
                box->n_publishers, box->n_subscribers);

        // Resets the box
        memset(box, 0, sizeof(Message_Box));
    }
    free(box);

    // If there is no boxes
    if (n == 0) {
        fprintf(stdout, "NO BOXES FOUND\n");
    }

    unlink(reg_pipe);

    close(session_fd);
}

/**
 * @brief Open the Pipe to read and print in stdout if the box will be created
 * or not if there is
 */
void update_boxes() {
    // Open the pipe
    session_fd = open(reg_pipe, O_RDONLY | O_APPEND);
    if (session_fd < 0) {
        perror("Error while opening fifo at manager");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the response
    response = (Box_Protocol *)malloc(sizeof(Box_Protocol));

    // Read the box protocol of the session
    if (read(session_fd, response, sizeof(Box_Protocol)) < 0) {
        free(response);
        perror("Error while reading manager fifo");
        exit(EXIT_FAILURE);
    }

    // When we create the box in create_box and it is created successfully
    // it updates the value of the response to 0. Here we check if the value is
    // 0 if created or not in other way.
    if (response->response == 0) {
        fprintf(stdout, "OK\n");
    } else {
        fprintf(stdout, "ERROR %s\n", response->error_message);
    }

    free(response);

    unlink(reg_pipe);

    close(session_fd);
}

void ping_ping() {
    session_fd = open(reg_pipe, O_RDONLY);
    if (session_fd < 0) {
        perror("Error while opening fifo at manager");
        exit(EXIT_FAILURE);
    }

    char buffer[1];

    if (read(session_fd, buffer, 1) < 0) {
        perror("Error while opening fifo at manageraaaaaaaaa");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "%s\n", buffer);

    unlink(reg_pipe);
    close(session_fd);
}

int main(int argc, char **argv) {
    // Sets handlers for signals
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
    }
    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
    }
    if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
    }

    if (argc != 4 && argc != 5) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    reg_pipe = argv[1];
    char *pipe_name = argv[2];
    char *action = argv[3];

    // Opens the server fifo
    server_fd = open(pipe_name, O_WRONLY | O_APPEND);
    if (server_fd < 0) {
        fprintf(stderr, "Error while opening server fifo at manager");
        raise(SIGTERM);
    }

    // Allocs memory for the registry
    registry = (Registry_Protocol *)malloc(sizeof(Registry_Protocol));

    if (argc == 4) {
        if (strcmp(action, "list") == 0) {
            // List code -> 7
            registry->code = 7;
            strcpy(registry->register_pipe_name, reg_pipe);
        } else if (strcmp(action, "ping") == 0) {
            registry->code = 20;
            strcpy(registry->register_pipe_name, reg_pipe);
        } else {
            print_usage();
            raise(SIGTERM);
        }

    } else {
        // Copy infos to registry
        strcpy(registry->register_pipe_name, reg_pipe);
        strcat(registry->box_name, "/");
        strcat(registry->box_name, argv[4]);

        if (strcmp(action, "create") == 0) {
            // List code -> 3
            registry->code = 3;
        } else if (strcmp(action, "remove") == 0) {
            // List code -> 5
            registry->code = 5;
        } else {
            print_usage();
            raise(SIGTERM);
        }
    }

    // Creates client session fifo
    if (mkfifo(reg_pipe, 0777)) {
        fprintf(stderr, "Error while making manager fifo");
        raise(SIGTERM);
    }

    // Write the registry protocol in the server file
    if (write(server_fd, registry, sizeof(Registry_Protocol)) < 0) {
        fprintf(stderr, "Error while writing in fifo");
        raise(SIGTERM);
    }

    close(server_fd);

    // Request to delete a box
    if (registry->code == 7) {
        free(registry);
        list_boxes();
    } else if (registry->code == 20) {
        free(registry);
        ping_ping();
    } else {
        free(registry);
        update_boxes();
    }

    unlink(reg_pipe);

    return 0;
}
