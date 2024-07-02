#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define FIFO_PATH "/tmp/daemon_fifo"

int send_add_user_to_group_request(const char *username, const char *group_name) {
    int fifo_fd;
    char buffer[512];

    snprintf(buffer, sizeof(buffer), "ADD_USER_TO_GROUP:%s:%s", username, group_name);

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
        fprintf(stderr, "Usage: %s <group_name> <username>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (send_add_user_to_group_request(argv[1], argv[2]) != 0) {
        fprintf(stderr, "Failed to send add user to group request\n");
        return EXIT_FAILURE;
    }

    printf("Request to add user '%s' to group '%s' sent successfully.\n", argv[1], argv[2]);
    return EXIT_SUCCESS;
}