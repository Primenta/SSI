#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define FIFO_PATH "/tmp/daemon_fifo"

int send_list_group_members_request(const char *group_name) {
    int fifo_fd;
    char buffer[512];

    snprintf(buffer, sizeof(buffer), "LIST_GROUP_MEMBERS:%s", group_name);

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
        fprintf(stderr, "Usage: %s <group_name>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (send_list_group_members_request(argv[1]) != 0) {
        fprintf(stderr, "Failed to send list group members request\n");
        return EXIT_FAILURE;
    }

    printf("Request to list members of group '%s' sent successfully.\n", argv[1]);
    return EXIT_SUCCESS;
}