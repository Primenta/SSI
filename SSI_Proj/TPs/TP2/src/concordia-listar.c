#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FIFO_PATH "/tmp/daemon_fifo"

int send_list_command(int all) {
    int fifo_fd;
    char command[512];

    snprintf(command, sizeof(command), "LIST_MESSAGE:%s", all ? "-a" : "");

    fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        perror("Erro ao abrir o FIFO");
        return -1;
    }

    if (write(fifo_fd, command, strlen(command) + 1) == -1) {
        perror("Erro ao escrever no FIFO");
        close(fifo_fd);
        return -1;
    }

    close(fifo_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    int all = argc == 2 && strcmp(argv[1], "-a") == 0;

    if (send_list_command(all) != 0) {
        fprintf(stderr, "Falha ao enviar comando de listagem ao daemon\n");
        return EXIT_FAILURE;
    }

    printf("Comando de listagem enviado com sucesso.\n");
    return EXIT_SUCCESS;
}