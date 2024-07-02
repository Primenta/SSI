CC = gcc
CFLAGS = -Wall
BIN_DIR = bin
SRC_DIR = src

EXECUTABLES = daemon concordia-ativar concordia-desativar concordia-enviar concordia-grupo-criar concordia-grupo-destinatario-adicionar concordia-grupo-destinatario-remover concordia-grupo-listar concordia-grupo-remover concordia-ler concordia-listar concordia-remover concordia-responder

all: $(EXECUTABLES)

daemon:
	$(CC) $(CFLAGS) -o $(BIN_DIR)/daemonexe $(SRC_DIR)/daemon.c

$(filter-out daemon, $(EXECUTABLES)):
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $(SRC_DIR)/$@.c

setup-daemon: daemon
	sudo chown mydaemon $(BIN_DIR)/daemonexe
	sudo chmod 755 $(BIN_DIR)/daemonexe

clean:
	rm -f $(BIN_DIR)/*

.PHONY: all clean setup-daemon

