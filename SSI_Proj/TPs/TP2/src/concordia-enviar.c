#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define FIFO_PATH "/tmp/daemon_fifo"

int send_message_to_daemon(const char *sender, const char *recipient, const char *message) {
    int fifo_fd;
    char buffer[512];

    snprintf(buffer, sizeof(buffer), "SEND_MESSAGE:%s:%s:%s", sender, recipient, message);

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
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <recipient> <message>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *sender = getenv("USER");
    if (sender == NULL) {
        fprintf(stderr, "Failed to determine the sender\n");
        return EXIT_FAILURE;
    }

    if (send_message_to_daemon(sender, argv[1], argv[2]) != 0) {
        fprintf(stderr, "Failed to send message\n");
        return EXIT_FAILURE;
    }

    printf("Message sent successfully from '%s' to '%s'.\n", sender, argv[1]);
    return EXIT_SUCCESS;
}
