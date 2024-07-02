# Segurança de Sistemas Informáticos - TP1 
### GRUPO 23

Neste projeto, desenvolvemos um serviço de envio de mensagens, *MSG RELAY SERVICE*, no qual pretendemos garantir a segurança das comunicações. De modo a certificar isso, implementamos um sistema de troca de chaves baseado no algoritmo *Diffie-Hellman*. 

Para além disso, incluimos na criptografia de mensagens o modo *AES-GCM*, a autenticação com certificados digitais e comunicação assíncrona entre cliente e servidor.

Quanto à vantagens dos principais métodos adotados e lecionados nas aulas, a combinação do *AES-GCM* para criptografia de mensagens e o protocolo *Diffie-Hellman* para troca de chaves oferece segurança para as comunicações entre cliente e servidor. Isto garante então que as mensagens sejam confidenciais, autenticadas e protegidas contra manipulações durante a transmissão.

## Estrutura do Projeto

- **msg_client.py**: Implementa a funcionalidade do cliente, incluindo a gestão da comunicação, a criptografia de mensagens e a autenticação com certificados digitais.

- **msg_server.py**: Implementa a funcionalidade do servidor, incluindo a gestão das conexões, o processamento de mensagens enviadas e a troca das chaves criptográficas.

- **verify_cert.py**: Neste ficheiro, incluímos funções lecionadas em aula relacionadas à validação de certificados digitais.

## Funcionalidades

- **Gestão de Comunicação**: O cliente e o servidor comunicam-se por meio de uma conexão *TCP/IP*. O cliente envia solicitações ao servidor e recebe respostas em troca.

- **Troca de Chaves *Diffie-Hellman***: Antes de comunicarem entre si de forma segura, o cliente e o servidor, realizam uma troca de chaves através do método de *Diffie-Hellman* para estabelecer uma chave compartilhada, tal como mencionado previamente.

- **Criptografia das Mensagens**: Após a troca de chaves, as mensagens são criptografadas usando o algoritmo *AES-GCM* para garantir a confidencialidade e integridade das comunicações.

- **Autenticação com Certificados Digitais**: Os certificados digitais são usados para autenticar e verificar a identidade do cliente e do servidor durante o processo de conexão.

- **Comunicação Assíncrona**: Toda a comunicação é realizada de forma assíncrona, o que permite que o cliente e o servidor lidem com várias conexões simultâneas de forma eficiente.

## Utilização
1. Executar o servidor a partir o ficheiro `msg_server.py`.
2. Executar o cliente a partir o ficheiro `msg_client.py`.
3. Enviar, por exemplo, uma mensagem: `send <UID> <Assunto>`, após efetuar esse comando, digitar a mensagem que pretende enviar.
4. O comando `askqueue` vai exibir as mensagens na fila de um cliente. 
5. O comando `getmsg <NUM>` vai buscar à *queue* do cliente a mensagem.
6. O comando `help` exibe os comandos disponíveis.
6. O comando `quit` sai do programa.
7. O cliente envia mensagens ao servidor, e o servidor responderá conforme necessário.
