#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define FIFO_PATH "/tmp/daemon_fifo"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <message_index> <response_message>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *message_index = argv[1];
    const char *response_message = argv[2];

    char command[1024];
    int fifo_fd;

    fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        fprintf(stderr, "Failed to open FIFO %s: %s\n", FIFO_PATH, strerror(errno));
        return EXIT_FAILURE;
    }

    snprintf(command, sizeof(command), "concordia-responder:%s:%s", message_index, response_message);

    if (write(fifo_fd, command, strlen(command) + 1) == -1) {
        fprintf(stderr, "Failed to write to FIFO %s: %s\n", FIFO_PATH, strerror(errno));
        close(fifo_fd);
        return EXIT_FAILURE;
    }

    close(fifo_fd);
    return EXIT_SUCCESS;
}