#include "logger.hpp"
#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstdlib>
#include <ctime>

#define PORT 7000
#define BUFFER_SIZE 1024

int sock;
pthread_mutex_t sock_mutex = PTHREAD_MUTEX_INITIALIZER;

void *receber_mensagem(void *arg)
{
    char buffer[BUFFER_SIZE];
    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);

        pthread_mutex_lock(&sock_mutex);
        int bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        pthread_mutex_unlock(&sock_mutex);

        if (bytes <= 0)
        {
            log_info("[CLIENTE] - Servidor desconectado!");
            break;
        }

        buffer[bytes] = '\0';
        log_info("[CLIENTE] Recebido: " + std::string(buffer));
    }
    return nullptr;
}

int main()
{
    srand(time(NULL) ^ getpid()); // random diferente em cada processo

    struct sockaddr_in server_address;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        log_error("[CLIENTE] - Erro ao criar socket");
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0)
    {
        log_error("[CLIENTE] - Endereço inválido");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        log_error("[CLIENTE] - Conexão falhou!");
        return -1;
    }

    log_info("[CLIENTE] Conectado ao servidor!");

    pthread_t t_recv;
    pthread_create(&t_recv, nullptr, receber_mensagem, nullptr);

    // Thread principal só manda mensagens
    while (true)
    {
        int num = rand() % 10;
        std::string mensagem = "[CLIENTE] Numero gerado: " + std::to_string(num) + "\n";

        pthread_mutex_lock(&sock_mutex);
        send(sock, mensagem.c_str(), mensagem.size(), 0);
        pthread_mutex_unlock(&sock_mutex);

        if (num == 0)
        {
            log_info("[CLIENTE] Saindo...");
            pthread_mutex_lock(&sock_mutex);
            shutdown(sock, SHUT_RDWR); // força recv a retornar
            close(sock);
            pthread_mutex_unlock(&sock_mutex);
            break;
        }

        sleep(1); // intervalo entre mensagens
    }

    return 0;
}
