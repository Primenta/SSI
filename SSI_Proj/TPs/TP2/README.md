# Segurança de Sistemas Informáticos
## Grupo 23
                        Miguel Tomás Antunes Pinto    a100815

                        Ana Margarida Sousa Pimenta   a100830

                        Pedro Miguel da Costa Azevedo a100557

## Introdução
No âmbito do projeto relativo à UC de Segurança de Sistemas Informáticos, foi-nos proposta a implementação de um serviço de conversação entre os utilizadores locais de um sistema Linux.

Neste relatório, iremos explorar e abordar os métodos adotados no nosso processo de realização do trabalho, bem como explicar as soluções encontradas e utilizadas nesta tarefa.

De modo a desenvolver este programa, recorremos aos conceitos lecionados e a informação obtida a partir de pesquisa efetuada pelo grupo. 

## Configurações Visudo

Após ter o utilizador `mydaemon` criado, fazemos as seguintes configurações:

- `mydaemon ALL=(ALL) NOPASSWD: /usr/sbin/groupadd, /usr/sbin/usermod, /usr/bin/getent, /usr/sbin/groupdel`

## Método de utilização do programa

- `sudo make`
- `sudo -u mydaemon ./bin/daemonexe` (mydaemon é o user criado com este propósito)
- `./bin/concordia_grupo_criar <nome_do_grupo>` (depois compilamos os executáveis com os respetivos parâmetros) 

- `make clean` 
