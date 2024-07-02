#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/daemon_fifo"

int send_deactivation_request(const char *username) {
    int fifo_fd;
    char buffer[512];

    snprintf(buffer, sizeof(buffer), "DEACTIVATE:%s", username);

    fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        perror("Erro ao abrir o FIFO");
        return -1;
    }

    if (write(fifo_fd, buffer, strlen(buffer) + 1) == -1) {
        perror("Erro ao escrever no FIFO");
        close(fifo_fd);
        return -1;
    }

    close(fifo_fd);
    return 0;
}

int main() {
    char *username = getenv("USER");
    if (username == NULL) {
        fprintf(stderr, "Erro ao obter o nome do usuário\n");
        return EXIT_FAILURE;
    }

    if (send_deactivation_request(username) != 0) {
        fprintf(stderr, "Falha ao enviar pedido de desativação ao daemon\n");
        return EXIT_FAILURE;
    }

    printf("Pedido de desativação enviado para o usuário '%s'.\n", username);
    return EXIT_SUCCESS;
}