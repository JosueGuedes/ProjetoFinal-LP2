#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include "logger.hpp"

#define PORT 7000
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
// #define LOGGER_TXT "logs.txt"

std::vector<int> clientes;
pthread_mutex_t mutex_clientes = PTHREAD_MUTEX_INITIALIZER;

void mandar_mensagem(const std::string message, int socket_mandante)
{
    pthread_mutex_lock(&mutex_clientes);
    const string msg_com_emissario = std::string("[Socket ") + to_string(socket_mandante) + std::string("]: ") + message;

    for (int cliente : clientes)
    {
        if (cliente != socket_mandante)
            send(cliente, msg_com_emissario.c_str(), msg_com_emissario.size(), 0);
    }
    pthread_mutex_unlock(&mutex_clientes);
}

void *lidar_cliente(void *arg)
{
    int socket_cliente = *(int *)arg;
    char buffer[BUFFER_SIZE];
    delete (int *)arg;

    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(socket_cliente, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
        {
            log_debug("[SERVER] Cliente Desconectado");

            // Remove cliente da lista
            pthread_mutex_lock(&mutex_clientes);
            clientes.erase(std::remove(clientes.begin(), clientes.end(), socket_cliente), clientes.end());
            pthread_mutex_unlock(&mutex_clientes);

            close(socket_cliente);
            pthread_exit(nullptr);
        }

        buffer[bytes] = '\0';
        std::string mensagem = std::string("[SERVER] Cliente de socket ") + to_string(socket_cliente) + std::string(" mandou mensagem. Mensagem recebida: ") + std::string(buffer);
        log_info(mensagem);
        mandar_mensagem(buffer, socket_cliente);
    }

    return NULL;
}

int main()
{
    // Configurando servidor
    int server_fd, novo_socket;
    struct sockaddr_in endereco{};
    socklen_t tam_endereco = sizeof(endereco);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        log_error("[SERVER] - Socket falhou!");
        perror("Socket falhou!");
        return 1;
    }

    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&endereco, sizeof(endereco)) < 0)
    {
        perror("Bind falhou!");
        log_error("[SERVER] - Bind falhou!");
        return 1;
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("Listen falhou");
        log_error("[SERVER] - Listen fahou!");
    }

    std::string mensagem_inicial = std::string("[SERVER] Servidor escutando na porta ") + to_string(PORT) + std::string(". . .\n");
    log_debug(mensagem_inicial);

    while (true)
    {
        novo_socket = accept(server_fd, (struct sockaddr *)&endereco, &tam_endereco);
        if (novo_socket < 0)
        {
            log_warn("[SERVER] - Accept falhou");
            perror("Accept falhou");
            continue;
        }

        std::string mensagem_join = std::string("[SERVER] Novo cliente de socket ") + to_string(novo_socket) + std::string(" conectou!");
        log_info(mensagem_join);

        // Zona crítica: Acesso a variavel global
        pthread_mutex_lock(&mutex_clientes);
        clientes.push_back(novo_socket);
        pthread_mutex_unlock(&mutex_clientes);

        // Criação de thread
        pthread_t tid;
        int *pcliente = new int(novo_socket);
        pthread_create(&tid, nullptr, lidar_cliente, pcliente);
        pthread_detach(tid);
    }
}
