#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/daemon_fifo"

int send_remove_request(const char *username, int message_id) {
    int fifo_fd;
    char buffer[512];
a 
    snprintf(buffer, sizeof(buffer), "REMOVE_MESSAGE:%s:%d", username, message_id);

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

    int message_id = atoi(argv[1]);
    if (send_remove_request(username, message_id) != 0) {
        fprintf(stderr, "Failed to send remove request\n");
        return EXIT_FAILURE;
    }

    printf("Remove request for message ID '%d' sent successfully.\n", message_id);
    return EXIT_SUCCESS;
}
