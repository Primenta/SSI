#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define FIFO_PATH "/tmp/daemon_fifo"

int send_remove_user_from_group_request(const char *group_name, const char *username) {
    int fifo_fd;
    char buffer[512];

    snprintf(buffer, sizeof(buffer), "REMOVE_USER_FROM_GROUP:%s:%s", username, group_name);

    fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        perror("Error opening FIFO");
        return -1;
    }

    // Escreve a mensagem na FIFO
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
        fprintf(stderr, "Usage: %s <group_name> <username>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (send_remove_user_from_group_request(argv[1], argv[2]) != 0) {
        fprintf(stderr, "Failed to send remove user from group request\n");
        return EXIT_FAILURE;
    }

    printf("Request to remove user '%s' from group '%s' sent successfully.\n", argv[1], argv[2]);
    return EXIT_SUCCESS;
}