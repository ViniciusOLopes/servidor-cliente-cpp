// Código para compilar: g++ -pthread cliente.cpp -o cliente
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>

#define BUFFER_LENGTH 4096

using namespace std;

int sockfd;
bool rodando = true;

// Thread auxiliar para receber mensagens do servidor 
void* thread_recebedora(void* arg) {
    char buffer[BUFFER_LENGTH];
    while (rodando) {
        memset(buffer, 0, BUFFER_LENGTH);
        int bytes = recv(sockfd, buffer, BUFFER_LENGTH, 0);
        if (bytes <= 0) {
            printf("\nConexão encerrada pelo servidor.\n");
            rodando = false;
            break;
        }

        string pacote(buffer);
        size_t pos_dado = pacote.find("|", pacote.find("|") + 1) + 1;
        size_t pos_fim = pacote.find("|eom");
        
        if (pos_dado != string::npos && pos_fim != string::npos) {
            string msg = pacote.substr(pos_dado, pos_fim - pos_dado);
            printf("\n>> %s\n", msg.c_str());
        }
    }
    return NULL;
}

int main() {
    string ip;
    int port;
    string apelido;

    // Solicita IP e Porta 
    cout << "Digite o IP do servidor: ";
    cin >> ip;
    cout << "Digite a porta do servidor: ";
    cin >> port;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(sockfd, (struct sockaddr*)&server, sizeof(server)) == -1) {
        perror("Erro ao conectar");
        return 1;
    }

    // Recebe boas-vindas do servidor 
    char buffer[BUFFER_LENGTH];
    recv(sockfd, buffer, BUFFER_LENGTH, 0);
    string boas_vindas(buffer);
    size_t pos1 = boas_vindas.find("|", boas_vindas.find("|") + 1) + 1;
    size_t pos2 = boas_vindas.find("|eom");
    cout << boas_vindas.substr(pos1, pos2 - pos1) << endl;

    // Solicita apelido e envia conexão 
    cout << "Digite seu apelido: ";
    cin >> apelido;
    cin.ignore(); // Limpa o buffer do teclado
    
    string cmd_entra = "bom|usuario_entra|" + apelido + "|eom";
    send(sockfd, cmd_entra.c_str(), cmd_entra.length(), 0);

    // Cria thread para ouvir o servidor 
    pthread_t t_recv;
    pthread_create(&t_recv, NULL, thread_recebedora, NULL);

    // Loop principal (envia mensagens) 
    while (rodando) {
        string mensagem;
        getline(cin, mensagem);

        if (mensagem == "tchau") { 
            string cmd_sai = "bom|usuario_sai|" + apelido + "|eom";
            send(sockfd, cmd_sai.c_str(), cmd_sai.length(), 0);
            rodando = false;
            break;
        } else if (mensagem.length() > 0) {
            string cmd_msg = "bom|msg_cliente|" + mensagem + "|eom";
            send(sockfd, cmd_msg.c_str(), cmd_msg.length(), 0);
        }
    }

    close(sockfd);
    return 0;
}