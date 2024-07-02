#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define FIFO_PATH "/tmp/daemon_fifo"

int send_read_request_to_daemon(const char *username, const char *message_id) {
    int fifo_fd;
    char buffer[512];

    snprintf(buffer, sizeof(buffer), "READ_MESSAGE:%s:%s", username, message_id);

    fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        perror("Error opening FIFO");
        return -1;
    }

    if (write(fifo_fd, buffer, strlen(buffer) + 1) == -1) {
        perror("Error writing to FIFO");
        close(fifo_fd);
        return -1;
    }

    close(fifo_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <message_id>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *username = getenv("USER");
    if (username == NULL) {
        fprintf(stderr, "Failed to determine the user\n");
        return EXIT_FAILURE;
    }

    if (send_read_request_to_daemon(username, argv[1]) != 0) {
        fprintf(stderr, "Failed to send read request\n");
        return EXIT_FAILURE;
    }

    printf("Read request for message ID '%s' sent successfully.\n", argv[1]);
    return EXIT_SUCCESS;
}
