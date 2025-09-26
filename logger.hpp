#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

pthread_mutex_t logger_mutex = PTHREAD_MUTEX_INITIALIZER;
ofstream logger_file("logs.txt");

void write_log(const std::string &msg)
{
    logger_file << msg;
}

void log_info(const std::string &msg)
{
    pthread_mutex_lock(&logger_mutex);
    std::string log = "[ERROR] " + msg + std::string("\n");
    cout << log;
    write_log(log);
    pthread_mutex_unlock(&logger_mutex);
}

void log_warn(const std::string &msg)
{
    pthread_mutex_lock(&logger_mutex);
    std::string log = "[WARN] " + msg + std::string("\n");
    cout << log;
    write_log(log);
    pthread_mutex_unlock(&logger_mutex);
}

void log_debug(const std::string &msg)
{
    pthread_mutex_lock(&logger_mutex);
    std::string log = "[DEBUG] " + msg + std::string("\n");
    cout << log;
    write_log(log);
    pthread_mutex_unlock(&logger_mutex);
}

void log_error(const std::string &msg)
{
    pthread_mutex_lock(&logger_mutex);
    std::string log = "[ERROR] " + msg + std::string("\n");
    cout << log;
    write_log(log);
    pthread_mutex_unlock(&logger_mutex);
}

#endif