#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/daemon_fifo"

int send_create_group_request(const char *group_name) {
    int fifo_fd;
    char buffer[512];

    snprintf(buffer, sizeof(buffer), "CREATE_GROUP:%s", group_name);

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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <nome_do_grupo>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (send_create_group_request(argv[1]) != 0) {
        fprintf(stderr, "Falha ao enviar pedido de criação de grupo ao daemon\n");
        return EXIT_FAILURE;
    }

    printf("Pedido de criação do grupo '%s' enviado com sucesso.\n", argv[1]);
    return EXIT_SUCCESS;
}