#include "logger.hpp"
#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstdlib>

#define PORT 7000
#define BUFFER_SIZE 1024

int sock;

void *receber_mensagem(void *arg)
{
    char buffer[BUFFER_SIZE];
    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);

        // Servidor desconectou-se
        if (bytes <= 0)
        {
            log_info("[CLIENTE] - Servidor desconectado!");
            close(sock);
            exit(0);
        }

        buffer[bytes] = '\0';
        log_info("[CLIENTE] Mensagem recebida: " + std::string(buffer));
    }

    return nullptr;
}

int main()
{
    struct sockaddr_in server_address;
    // char buffer[BUFFER_SIZE] = {0};

    // Criação de socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        log_error("[CLIENTE] - Erro ao criar socket");
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Configuração de IP (Localhost)
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0)
    {
        log_error("[CLIENTE] - Endereço invalido");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        log_error("[CLIENTE] - Conexão falhou!");
        return -1;
    }

    int id_sala = -1;
    do
    {
        std::cout << ">>> Digite o id da sala (0 - 4): ";
        std::cin >> id_sala;
        std::cin.ignore(); // limpa o buffer
    } while (id_sala < 0 || id_sala > 4);

    // Envia o ID da sala ao servidor
    int id_sala_network = htonl(id_sala);
    if(send(sock, &id_sala_network, sizeof(id_sala_network), 0) < 0){
        log_error("[CLIENTE] - Erro ao enviar id da sala");
        close(sock);
        return -1;
    }
    

    log_info("[CLIENTE] - Conectado ao servidor!");

    pthread_t thread_receptora;
    pthread_create(&thread_receptora, nullptr, receber_mensagem, nullptr);
    pthread_detach(thread_receptora);

    while (true)
    {
        std::string mensagem;
        cout << ">> Digite a mensagem: ";
        std::getline(std::cin, mensagem);

        if (mensagem == "/sair")
        {
            log_info("[CLIENTE] Saindo . . .");
            close(sock);
            break;
        }

        send(sock, mensagem.c_str(), mensagem.size(), 0);
    }

    return 0;
}
