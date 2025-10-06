#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <cstring>
#include <string>
#include <queue>
#include <map>
#include <atomic>
#include <vector>
#include <algorithm>
#include "logger.hpp"

#define PORT 7000
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define NUM_SALAS 5
// #define LOGGER_TXT "logs.txt"

struct chat
{
    int id;
    std::vector<int> clientes;
    std::queue<std::string> historico_mensagens;
    pthread_mutex_t mutex_clientes;
    pthread_mutex_t mutex_historico;
    pthread_cond_t cond_new_msg;

    chat()
    {
        pthread_mutex_init(&mutex_clientes, nullptr);
        pthread_mutex_init(&mutex_historico, nullptr);
        pthread_cond_init(&cond_new_msg, nullptr);
    }

    ~chat()
    {
        pthread_mutex_destroy(&mutex_clientes);
        pthread_mutex_destroy(&mutex_historico);
        pthread_cond_destroy(&cond_new_msg);
    }
};

std::vector<chat *> salas;
pthread_mutex_t mutex_salas = PTHREAD_MUTEX_INITIALIZER;
sem_t chatroom_init;
volatile sig_atomic_t running = 1;
int server_fd = -1;

// Utilizados para limpeza posterior
std::vector<pthread_t> client_threads;
std::vector<pthread_t> broadcaster_threads;
pthread_mutex_t client_threads_mutex = PTHREAD_MUTEX_INITIALIZER;

// handler de sinal
void handle_sigint(int signo)
{
    (void)signo;
    running = 0;
    if (server_fd != -1)
    {
        // força accept() a retornar
        shutdown(server_fd, SHUT_RDWR);
        // não close aqui, main fará close depois
    }
}

void *initChatRoom(void *arg)
{
    int id_sala = *(int *)arg;
    delete (int *)arg; // libera a memória alocada

    chat *novo_chat = new chat();
    novo_chat->id = id_sala;
    pthread_mutex_lock(&mutex_salas);
    salas[id_sala] = novo_chat;
    pthread_mutex_unlock(&mutex_salas);
    sem_post(&chatroom_init);
    return nullptr;
}

chat *obterSala(int id_sala)
{
    // Verifica se o ID é válido
    if (id_sala < 0 || id_sala >= (int)salas.size())
    {
        log_error("[SERVER] ID de sala invalido");
        std::cerr << "[ERRO] ID de sala inválido: " << id_sala << std::endl;
        return nullptr;
    }

    // opcional: lock se você quer garantir sincronização com writes (initChatRoom)
    pthread_mutex_lock(&mutex_salas);
    chat *ptr = salas[id_sala];
    pthread_mutex_unlock(&mutex_salas);

    // ptr não deve ser nullptr se main aguardou semáforo
    if (!ptr)
    {
        log_error("[SERVER] Sala não foi inicializada");
        std::cerr << "[ERRO] Sala ainda nao inicializada: " << id_sala << std::endl;
    }
    return ptr;
}

void *mandar_mensagem(void *arg)
{
    chat *sala = (chat *)arg;
    while (running)
    {
        pthread_mutex_lock(&sala->mutex_historico);
        while (sala->historico_mensagens.empty() && running)
        {
            pthread_cond_wait(&sala->cond_new_msg, &sala->mutex_historico);
        }

        if (!running)
        {
            pthread_mutex_unlock(&sala->mutex_historico);
            break;
        }

        // Pegar a próxima mensagem
        std::string msg = sala->historico_mensagens.front();
        sala->historico_mensagens.pop();
        pthread_mutex_unlock(&sala->mutex_historico);

        // Enviar a todos os clientes conectados
        pthread_mutex_lock(&sala->mutex_clientes);
        for (int cliente_sock : sala->clientes)
        {
            if (cliente_sock > 0)
            {
                send(cliente_sock, msg.c_str(), msg.size(), 0);
            }
        }
        pthread_mutex_unlock(&sala->mutex_clientes);
    }

    return nullptr;
}

void *lidar_cliente(void *arg)
{
    int socket_cliente = *(int *)arg;
    char buffer[BUFFER_SIZE];
    delete (int *)arg;

    int id_sala;
    int bytes = recv(socket_cliente, &id_sala, sizeof(id_sala), 0);
    if (bytes <= 0)
    {
        log_error("[SERVER] Falha ao receber id_sala do cliente");
        close(socket_cliente);
        return nullptr;
    }

    id_sala = ntohl(id_sala);
    chat *sala = obterSala(id_sala);
    if (!sala)
    {
        log_error("[SERVER] Sala inválida solicitada pelo cliente");
        close(socket_cliente);
        return nullptr;
    }

    pthread_mutex_lock(&sala->mutex_clientes);
    sala->clientes.push_back(socket_cliente);
    pthread_mutex_unlock(&sala->mutex_clientes);

    while (running)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(socket_cliente, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
        {
            log_debug("[SERVER] Cliente Desconectado");

            // Remove cliente da lista
            pthread_mutex_lock(&sala->mutex_clientes);
            sala->clientes.erase(std::remove(sala->clientes.begin(), sala->clientes.end(), socket_cliente), sala->clientes.end());
            pthread_mutex_unlock(&sala->mutex_clientes);

            close(socket_cliente);
            pthread_exit(nullptr);
        }

        // Preparando mensagem
        buffer[bytes] = '\0';
        std::string mensagem = std::string("[SERVER] Cliente de socket ") + to_string(socket_cliente) + std::string(" mandou mensagem. Mensagem recebida: ") + std::string(buffer);
        log_info(mensagem);

        std::string mensagem_com_enviante =
            std::string("[CHAT ") + to_string(sala->id) +
            std::string("][SOCKET ") + to_string(socket_cliente) + std::string("]: ") + std::string(buffer);

        pthread_mutex_lock(&sala->mutex_historico);
        sala->historico_mensagens.push(mensagem_com_enviante);
        pthread_mutex_unlock(&sala->mutex_historico);

        pthread_cond_signal(&sala->cond_new_msg);
    }

    return NULL;
}

int main()
{
    // Configurando servidor
    int novo_socket;
    struct sockaddr_in endereco{};
    socklen_t tam_endereco = sizeof(endereco);
    signal(SIGINT, handle_sigint);
    sem_init(&chatroom_init, 0, 0);

    // Threads de criação das salas
    pthread_t thread_salas[NUM_SALAS];
    salas.resize(NUM_SALAS, nullptr);
    for (int i = 0; i < NUM_SALAS; i++)
    {
        int *id = new int(i);
        pthread_create(&thread_salas[i], nullptr, initChatRoom, id);
        pthread_detach(thread_salas[i]);
    }

    for (int j = 0; j < NUM_SALAS; j++)
        sem_wait(&chatroom_init);

    log_info("[SERVER] - Salas criadas!");

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

    // Thread listener pra novas mensagens, um para cada chat
    broadcaster_threads.resize(NUM_SALAS);
    for (int k = 0; k < NUM_SALAS; k++)
    {
        chat *chatroom = salas[k];
        pthread_create(&broadcaster_threads[k], nullptr, mandar_mensagem, chatroom);
    }

    log_info("[SERVER] - Listeners para mensagens em cada sala feitos!");

    while (running)
    {
        novo_socket = accept(server_fd, (struct sockaddr *)&endereco, &tam_endereco);
        if (!running)
            break;
        if (novo_socket < 0)
        {
            if (errno == EINTR)
                continue;
            log_warn("[SERVER] - Accept falhou");
            perror("Accept falhou");
            continue;
        }

        std::string mensagem_join = std::string("[SERVER] Novo cliente de socket ") + to_string(novo_socket) + std::string(" conectou!");
        log_info(mensagem_join);

        // Criação de thread
        pthread_t tid;
        int *pcliente = new int(novo_socket);
        int rc = pthread_create(&tid, nullptr, lidar_cliente, pcliente);
        if (rc != 0)
        {
            log_error("[SERVER] Falha ao criar thread cliente");
            close(novo_socket);
            delete pcliente;
            continue;
        }

        // guardar tid thread para join posterior
        pthread_mutex_lock(&client_threads_mutex);
        client_threads.push_back(tid);
        pthread_mutex_unlock(&client_threads_mutex);
    }

    log_debug("[SERVER] Shutdown iniciado, fazendo limpeza . . .");

    // fechar server_fd (se ainda aberto)
    if (server_fd != -1)
    {
        close(server_fd);
        server_fd = -1;
    }

    // Para cada sala: fechar sockets dos clientes e sinalizar broadcasters
    for (int i = 0; i < (int)salas.size(); ++i)
    {
        chat *s = salas[i];
        if (!s)
            continue;

        pthread_mutex_lock(&s->mutex_clientes);
        // fazer cópia da lista para evitar modificar enquanto fecha
        std::vector<int> clientes_copy = s->clientes;
        s->clientes.clear();
        pthread_mutex_unlock(&s->mutex_clientes);

        for (int csock : clientes_copy)
        {
            if (csock > 0)
            {
                shutdown(csock, SHUT_RDWR);
                close(csock);
            }
        }

        // acordar broadcaster que pode estar esperando por mensagens
        pthread_mutex_lock(&s->mutex_historico);
        pthread_cond_broadcast(&s->cond_new_msg);
        pthread_mutex_unlock(&s->mutex_historico);
    }

    // juntar todas as threads de cliente
    pthread_mutex_lock(&client_threads_mutex);
    for (pthread_t t : client_threads)
    {
        pthread_join(t, nullptr);
    }
    client_threads.clear();
    pthread_mutex_unlock(&client_threads_mutex);

    // juntar broadcasters
    for (pthread_t bt : broadcaster_threads)
    {
        if (bt)
            pthread_join(bt, nullptr);
    }
    broadcaster_threads.clear();

    // deletar salas e destruir recursos
    for (chat *s : salas)
    {
        if (!s)
            continue;
        delete s; // destrói mutex/cond no destrutor
    }
    salas.clear();

    sem_destroy(&chatroom_init);
    log_info("[SERVER] Limpeza finalizada. Saindo.");
    return 0;
}
