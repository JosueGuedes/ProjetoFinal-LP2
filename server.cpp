#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include "logger.hpp"

#define PORT 7000
#define LOGGER_TXT "logs.txt"

void *testeThread(void *arg)
{
    int id = *((int *)arg);
    free(arg);
    std::string msg = std::string("Thread ") + std::to_string(id) + std::string(" escreveu!");
    log_info(msg);

    std::string msg_warn = std::string("Thread " + std::to_string(id) + std::string(" fez um warn!"));
    log_warn(msg_warn);

    pthread_exit(NULL);
}


int main()
{
    pthread_t threads[5];

        for (int i = 0; i < 5; i++)
    {
        int *id = (int *)malloc(sizeof(int));
        *id = i + 1;

        pthread_create(&threads[i], NULL, testeThread, id);
    }

    for (int j = 0; j < 5; j++)
        pthread_join(threads[j], NULL);

    return 0;
}
