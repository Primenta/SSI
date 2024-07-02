#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stddef.h>

#define RECEIVED_DIR "/home/%s/concordia/recebidas"
#define OUTPUT_FILE "/home/%s/concordia/todas_mensagens.txt"

#define FIFO_PATH "/tmp/daemon_fifo"
#define BUFFER_SIZE 512

void signal_handler(int signum) {
    if (signum == SIGTERM) {
        syslog(LOG_INFO, "Received SIGTERM, exiting.");
        unlink(FIFO_PATH);
        closelog();
        exit(EXIT_SUCCESS);
    }
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    umask(0);
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    openlog("daemon", LOG_PID, LOG_DAEMON);
    signal(SIGTERM, signal_handler);
}

int list_group_members(const char *group_name) {
    char *current_user = getlogin();
    if (current_user == NULL) {
        fprintf(stderr, "Falha ao obter o nome de usuário atual.\n");
        return -1;
    }

    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "/home/%s/concordia/tarefas_grupos/%s_members.txt", current_user, group_name);

    FILE *group_file = fopen("/etc/group", "r");
    FILE *output_file;
    char line[1024];
    char *name, *members;

    if (!group_file) {
        perror("Failed to open /etc/group");
        return -1;
    }

    while (fgets(line, sizeof(line), group_file)) {
        name = strtok(line, ":");
        strtok(NULL, ":");
        strtok(NULL, ":");
        members = strtok(NULL, "\n");

        if (strcmp(name, group_name) == 0) {
            fclose(group_file);
            output_file = fopen(output_path, "w");
            if (!output_file) {
                perror("Failed to open output file");
                return -1;
            }
            if (members) {
                fprintf(output_file, "Members of %s: %s\n", group_name, members);
            } else {
                fprintf(output_file, "No members found in group %s.\n", group_name);
            }
            fclose(output_file);
            return 0;
        }
    }

    fclose(group_file);
    fprintf(stderr, "Group '%s' not found.\n", group_name);
    return -1;
}

int is_first_member(const char *group_name, const char *username) {
    FILE *file = fopen("/etc/group", "r");
    if (!file) {
        perror("Failed to open /etc/group");
        return -1; 
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        char *name = strtok(line, ":");
        if (strcmp(name, group_name) == 0) {
            strtok(NULL, ":");
            strtok(NULL, ":");
            char *members = strtok(NULL, "\n");
            if (members != NULL) {
                char *first_member = strtok(members, ",");
                if (first_member != NULL && strcmp(first_member, username) == 0) {
                    fclose(file);
                    return 1;
                }
            }
            break;
        }
    }

    fclose(file);
    return 0;
}

int remove_user(const char *username, const char *group_name) {
    char *current_user = getlogin();
    if (current_user == NULL) {
        fprintf(stderr, "Falha ao obter o nome de usuário atual.\n");
        return -1;
    }

    if (!is_first_member(group_name, current_user)) {
        fprintf(stderr, "Erro: apenas o dono do grupo pode remover usuários do grupo '%s'.\n", group_name);
        return -1;
    }

    pid_t pid;
    int status;

    pid = fork();

    if (pid == -1) {
        perror("Falha ao criar processo");
        return -1;
    } else if (pid == 0) {
        execlp("gpasswd", "gpasswd", "-d", username, group_name, NULL);
        perror("Falha ao executar execlp");
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("Usuário '%s' removido do grupo '%s' com sucesso.\n", username, group_name);
            return 0;
        } else {
            fprintf(stderr, "Comando 'gpasswd' falhou com o código de saída %d\n", WEXITSTATUS(status));
            return -1;
        }
    }
}

int add_user_to_group(const char *username, const char *group_name) {
    char *current_user = getlogin();
    if (current_user == NULL) {
        fprintf(stderr, "Falha ao obter o nome de usuário atual.\n");
        return -1;
    }

    if (!is_first_member(group_name, current_user)) {
        fprintf(stderr, "Erro: apenas o dono do grupo pode adicionar usuários ao grupo '%s'.\n", group_name);
        return -1;
    }

    pid_t pid;
    int status;

    pid = fork();

    if (pid == -1) {
        perror("Falha ao criar processo");
        return -1;
    } else if (pid == 0) {
        execlp("usermod", "usermod", "-aG", group_name, username, (char *)NULL);
        perror("Falha ao executar execlp");
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("Usuário '%s' adicionado ao grupo '%s' com sucesso.\n", username, group_name);
            return 0;
        } else {
            fprintf(stderr, "Comando 'usermod' falhou com o código de saída %d\n", WEXITSTATUS(status));
            return -1;
        }
    }
}

int create_group(const char *group_name) {
    int status;
    pid_t pid;

    printf("Tentando criar o grupo: %s\n", group_name);

    pid = fork();

    if (pid == -1) {
        perror("Falha ao criar processo");
        return -1;
    } else if (pid == 0) {
        char *argv[] = {"sudo","groupadd", (char *)group_name, NULL};
        execvp("sudo", argv);
        perror("Falha ao executar comando");
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("Grupo '%s' criado com sucesso.\n", group_name);

            char *username = getlogin();
            if (username == NULL) {
                perror("Falha ao obter o nome do usuário");
                return -1;
            }

            pid = fork();
            if (pid == -1) {
                perror("Falha ao criar processo");
                return -1;
            } else if (pid == 0) {
                char *argv[] = {"usermod", "-aG", (char *)group_name, username, NULL};
                execvp("usermod", argv);
                perror("Falha ao executar usermod");
                exit(EXIT_FAILURE);
            } else {
                waitpid(pid, &status, 0);
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    printf("Usuário '%s' adicionado ao grupo '%s' com sucesso.\n", username, group_name);
                    return 0;
                } else {
                    fprintf(stderr, "Falha ao adicionar usuário '%s' ao grupo '%s'\n", username, group_name);
                    return -1;
                }
            }

        } else {
            fprintf(stderr, "Comando 'groupadd' falhou com o código de saída %d\n", WEXITSTATUS(status));
            return -1;
        }
    }
}

int remove_group(const char *group_name) {
    char *username = getlogin();
    if (username == NULL) {
        fprintf(stderr, "Erro ao obter o nome de usuário atual.\n");
        return -1;
    }

    if (!is_first_member(group_name, username)) {
        fprintf(stderr, "Erro: apenas o primeiro membro pode remover o grupo '%s'.\n", group_name);
        return -1;
    }

    pid_t pid;
    int status;

    pid = fork();
    if (pid == -1) {
        perror("Falha ao criar processo");
        return -1;
    } else if (pid == 0) {
        execlp("groupdel", "groupdel", group_name, NULL);
        perror("Falha ao executar execlp");
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("Grupo '%s' removido com sucesso.\n", group_name);
            return 0;
        } else {
            fprintf(stderr, "Comando 'groupdel' falhou com o código de saída %d\n", WEXITSTATUS(status));
            return -1;
        }
    }
}

int create_user_directories(const char *username) {
    char path[1024];
    snprintf(path, sizeof(path), "/home/%s/concordia", username);
    mkdir(path, 0761);
    snprintf(path, sizeof(path), "/home/%s/concordia/recebidas", username);
    mkdir(path, 0701);
    snprintf(path, sizeof(path), "/home/%s/concordia/tarefas_grupos", username);
    mkdir(path, 0761);
    return 0;
}

void process_remove_message(const char *senderLogged, const char *message_index) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/home/%s/concordia/recebidas/mensagem%s.txt", senderLogged, message_index);

    pid_t pid = fork();

    if (pid == -1) {
        perror("Failed to fork");
        return;
    }

    if (pid == 0) {
        execlp("rm", "rm", "-r", filepath, (char *)NULL);
        perror("Failed to exec rm");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("Message #%s removed successfully by %s.\n", message_index, senderLogged);
        } else {
            fprintf(stderr, "Failed to remove message file #%s by %s.\n", message_index, senderLogged);
        }
    }
}

int remove_directory(const char *path) {
    DIR *d = opendir(path);
    if (d == NULL) {
        syslog(LOG_ERR, "Failed to open directory: %s", path);
        return -1;
    }

    struct dirent *p;
    while ((p = readdir(d)) != NULL) {
        if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
            continue;

        char buf[1024];
        snprintf(buf, sizeof(buf), "%s/%s", path, p->d_name);
        struct stat statbuf;

        if (stat(buf, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                if (remove_directory(buf) == -1) {
                    syslog(LOG_ERR, "Failed to remove directory: %s", buf);
                    closedir(d);
                    return -1;
                }
            } else {
                if (unlink(buf) == -1) {
                    syslog(LOG_ERR, "Failed to unlink file: %s", buf);
                    closedir(d);
                    return -1;
                }
            }
        } else {
            syslog(LOG_ERR, "Failed to stat file: %s", buf);
        }
    }

    closedir(d);
    if (rmdir(path) == -1) {
        syslog(LOG_ERR, "Failed to remove directory: %s", path);
        return -1;
    }

    syslog(LOG_INFO, "Removed directory: %s", path);
    return 0;
}

int append_read_marker(const char *username, int message_id) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "/home/%s/concordia/recebidas/mensagem%d.txt", username, message_id);

    FILE *file = fopen(filepath, "a");
    if (file == NULL) {
        perror("Failed to open file for appending");
        return -1;
    }

    fprintf(file, "\nLIDO\n");
    fclose(file);

    printf("Marker 'LIDO' appended to message %d for user %s.\n", message_id, username);
    return 0;
}

void remove_message(const char *username, int message_id) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "/home/%s/concordia/recebidas/mensagem%d.txt", username, message_id);

    if (unlink(filepath) == -1) {
        syslog(LOG_ERR, "Failed to remove message file: %s, error: %s", filepath, strerror(errno));
    } else {
        syslog(LOG_INFO, "Message file %s removed successfully.", filepath);
    }
}

void list_messages(int include_all) {
    char received_path[512];
    char output_path[512];
    struct dirent *entry;
    FILE *file, *out_file;
    char buffer[1024];
    char line[1024];
    int count = 0;

    char *user = getenv("USER");
    if (!user) user = "default";

    snprintf(received_path, sizeof(received_path), RECEIVED_DIR, user);
    snprintf(output_path, sizeof(output_path), OUTPUT_FILE, user);

    DIR *dir = opendir(received_path);
    if (!dir) {
        perror("Erro ao abrir o diretório");
        return;
    }

    out_file = fopen(output_path, "w");
    if (!out_file) {
        perror("Erro ao criar o arquivo de saída");
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && (strncmp(entry->d_name, "mensagem", 8) == 0)) {
            snprintf(buffer, sizeof(buffer), "%s/%s", received_path, entry->d_name);
            if (include_all || count++ < 10) {
                file = fopen(buffer, "r");
                if (file) {
                    while (fgets(line, sizeof(line), file)) {
                        fputs(line, out_file);
                    }
                    fclose(file);
                }
            }
        }
    }

    fclose(out_file);
    closedir(dir);
    printf("Mensagens listadas em '%s'.\n", output_path);
}

void respond_to_message(int index, const char *response) {
    char original_file_path[BUFFER_SIZE];
    char response_file_path[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    FILE *original_file, *response_file;

    snprintf(original_file_path, sizeof(original_file_path), "/home/<user>/concordia/recebidas/mensagem%d.txt", index);
    snprintf(response_file_path, sizeof(response_file_path), "/home/<user>/concordia/recebidas/responder%d.txt", index);

    original_file = fopen(original_file_path, "r");
    if (original_file == NULL) {
        perror("Erro ao abrir o arquivo da mensagem original");
        return;
    }

    response_file = fopen(response_file_path, "w");
    if (response_file == NULL) {
        perror("Erro ao criar o arquivo de resposta");
        fclose(original_file);
        return;
    }

    while (fgets(buffer, BUFFER_SIZE, original_file) != NULL) {
        fputs(buffer, response_file);
    }

    fprintf(response_file, "\nResposta:\n%s\n", response);

    fclose(original_file);
    fclose(response_file);

    printf("Resposta salva em: %s\n", response_file_path);
}

void process_message(char *message, int fifo_fd) {
    char *cmd, *first_detail, *second_detail, *content;

    printf("Mensagem recebida: %s\n", message);

    static int message_count = -1;

    if (message_count == -1) {
        FILE *count_file = fopen("/tmp/message_count.txt", "r");
        if (count_file) {
            fscanf(count_file, "%d", &message_count);
            fclose(count_file);
        } else {
            message_count = 0;
        }
    }

    cmd = strtok(message, ":");
    first_detail = strtok(NULL, ":");
    second_detail = strtok(NULL, ":");
    content = strtok(NULL, "");

    if (cmd == NULL || first_detail == NULL) {
        syslog(LOG_ERR, "Invalid message format or missing command details: %s", message);
        return;
    }

    printf("Comando: %s, Primeiro detalhe: %s\n", cmd, first_detail);
    if (second_detail) {
        printf("Segundo detalhe: %s\n", second_detail);
    }
    if (content) {
        printf("Content: %s\n", content);
    }

    if (strcmp(cmd, "ACTIVATE") == 0) {
        create_user_directories(first_detail);
    } else if (strcmp(cmd, "DEACTIVATE") == 0) {
        char path[1024];
        snprintf(path, sizeof(path), "/home/%s/concordia", first_detail);
        remove_directory(path);
    } else if (strcmp(cmd, "CREATE_GROUP") == 0) {
        create_group(first_detail);
    } else if (strcmp(cmd, "REMOVE_GROUP") == 0) {
        remove_group(first_detail);
    } else if (strcmp(cmd, "ADD_USER_TO_GROUP") == 0 && second_detail) {
        add_user_to_group(second_detail, first_detail);
    } else if (strcmp(cmd, "REMOVE_USER") == 0) {
        remove_user(first_detail, second_detail);
    } else if (strcmp(cmd, "LIST_GROUP_MEMBERS") == 0) {
        list_group_members(first_detail);
    } else if (strcmp(cmd, "concordia-remover") == 0) {
        process_remove_message(first_detail, content);
    } else if (strcmp(cmd, "SEND_MESSAGE") == 0) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "/home/%s/concordia/recebidas/mensagem%d.txt", second_detail, message_count);

        FILE *file = fopen(filepath, "w");
        if (file == NULL) {
            perror("Failed to open file");
            return;
        }
        fprintf(file, "From: %s\nMessage: %s\n", first_detail, content);
        fclose(file);
        printf("Message saved to %s\n", filepath);

        message_count++;
        FILE *count_file = fopen("/tmp/message_count.txt", "w");
        if (count_file) {
            fprintf(count_file, "%d", message_count);
            fclose(count_file);
        }
    } else if (strcmp(cmd, "READ_MESSAGE") == 0){
        int message_id = atoi(second_detail);
        append_read_marker(first_detail, message_id);
    } else if (strcmp(cmd, "REMOVE_MESSAGE") == 0) {
        int message_id = atoi(second_detail);
        remove_message(first_detail, message_id);
    } else if (strcmp(cmd, "LIST_MESSAGES") == 0) {
        int list_all = atoi(first_detail);
        list_messages(list_all);
    } else if (strcmp(cmd, "concordia-responder") == 0) {
        int message_index = atoi(first_detail);
        respond_to_message(message_index, second_detail);
    } else {
        syslog(LOG_ERR, "Unrecognized command or invalid parameters: %s", cmd);
    }
}

int main() {
    daemonize();

    if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST) {
        syslog(LOG_ERR, "Failed to create FIFO: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) {
        syslog(LOG_ERR, "Failed to open FIFO: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t bytes_read = read(fifo_fd, buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            process_message(buffer, fifo_fd);
        }
        sleep(1);
    }

    return EXIT_SUCCESS;
}