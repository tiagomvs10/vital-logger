#ifndef RASPAPPCONNECTION_H
#define RASPAPPCONNECTION_H

#include <string>
#include <pthread.h>

class VitalLogger; // forward declaration

class RaspAppConnection
{
private:
    int listenPort; // tcp port
    VitalLogger *vitalLogger;
    std::string currentUserId;
    pthread_mutex_t idMutex;

public:
    RaspAppConnection(int listenPort, VitalLogger *loggerInstance);
    ~RaspAppConnection();

    void configure(std::string address, int port); // configure socket address
    std::string buildResponse(std::string responseData);
    void start();
    void stop();
    std::string receiveData();
    void sendData(std::string data);
    void processRequest(std::string request);
    std::string getCurrentUserId();
};

#endif