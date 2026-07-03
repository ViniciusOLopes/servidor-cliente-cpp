// Código para compilar: g++ -pthread servidor.cpp -o servidor
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <string>

#define MAX_CLIENTS 100
#define BUFFER_LENGTH 4096

using namespace std;

// Estrutura para manter clientes ativos
struct ClienteInfo {
    int fd;
    string apelido;
    bool ativo;
};

vector<ClienteInfo> clientes;
vector<string> buffer_mensagens;

pthread_mutex_t mutex_clientes = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_mensagens;

// Thread Consumidora: tira do buffer e envia para todos 
void* thread_consumidora(void* arg) {
    while (true) {
        sem_wait(&sem_mensagens);
        
        pthread_mutex_lock(&mutex_buffer);
        string msg = buffer_mensagens.front();
        buffer_mensagens.erase(buffer_mensagens.begin());
        pthread_mutex_unlock(&mutex_buffer);

        string pacote = "bom|msg_servidor|" + msg + "|eom";

        pthread_mutex_lock(&mutex_clientes);
        for (auto& c : clientes) {
            if (c.ativo) {
                send(c.fd, pacote.c_str(), pacote.length(), 0);
            }
        }
        pthread_mutex_unlock(&mutex_clientes);
    }
    return NULL;
}

// Extrai o dado de um pacote no formato bom|comando|dado|eom
string extrair_dado(string pacote, string& comando) {
    size_t pos1 = pacote.find('|');
    size_t pos2 = pacote.find('|', pos1 + 1);
    size_t pos3 = pacote.find('|', pos2 + 1);
    
    if (pos1 != string::npos && pos2 != string::npos && pos3 != string::npos) {
        comando = pacote.substr(pos1 + 1, pos2 - pos1 - 1);
        return pacote.substr(pos2 + 1, pos3 - pos2 - 1);
    }
    return "";
}

// Thread Produtora (Uma por cliente)
void* thread_cliente(void* arg) {
    int clientfd = *(int*)arg;
    free(arg);
    char buffer[BUFFER_LENGTH];
    string apelido = "";
    int index_cliente = -1;

    // Envia boas vindas
    string boas_vindas = "bom|msg_servidor|Olá! Seja bem-vindo!|eom";
    send(clientfd, boas_vindas.c_str(), boas_vindas.length(), 0);

    while (true) {
        memset(buffer, 0, BUFFER_LENGTH);
        int bytes = recv(clientfd, buffer, BUFFER_LENGTH, 0);
        if (bytes <= 0) break;

        string pacote(buffer);
        string comando;
        string dado = extrair_dado(pacote, comando);

        string msg_formatada = "";

        if (comando == "usuario_entra") {
            apelido = dado;
            msg_formatada = apelido + " entrou na sala de conversa.";
            
            pthread_mutex_lock(&mutex_clientes);
            ClienteInfo c = {clientfd, apelido, true};
            clientes.push_back(c);
            index_cliente = clientes.size() - 1;
            pthread_mutex_unlock(&mutex_clientes);
            
        } else if (comando == "usuario_sai") {
            msg_formatada = apelido + " saiu da sala de conversa.";
            if (index_cliente != -1) {
                pthread_mutex_lock(&mutex_clientes);
                clientes[index_cliente].ativo = false;
                pthread_mutex_unlock(&mutex_clientes);
            }
            break; // Sai do loop para finalizar thread
        } else if (comando == "msg_cliente") {
            msg_formatada = apelido + " enviou: " + dado; 
        }

        if (msg_formatada != "") {
            pthread_mutex_lock(&mutex_buffer);
            buffer_mensagens.push_back(msg_formatada);
            pthread_mutex_unlock(&mutex_buffer);
            sem_post(&sem_mensagens); // Sinaliza que há nova mensagem 
        }
    }

    close(clientfd);
    return NULL;
}

int main() {
    int port;
    printf("Digite a porta do servidor: "); 
    scanf("%d", &port);

    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    int yes = 1;
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    bind(serverfd, (struct sockaddr*)&server, sizeof(server));
    listen(serverfd, 10);

    sem_init(&sem_mensagens, 0, 0);

    pthread_t t_cons;
    pthread_create(&t_cons, NULL, thread_consumidora, NULL); 

    printf("Servidor rodando na porta %d...\n", port);

    while (true) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int* clientfd = (int*)malloc(sizeof(int));
        *clientfd = accept(serverfd, (struct sockaddr*)&client, &client_len);
        
        pthread_t t_prod;
        pthread_create(&t_prod, NULL, thread_cliente, clientfd); 
        pthread_detach(t_prod); 
    }

    close(serverfd);
    return 0;
}