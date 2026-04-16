#include "raspAppConnection.h"
#include "vitalLogger.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_SIZE 4096

static int server_fd = -1;
static int new_socket = -1;
static struct sockaddr_in address;

// auxiliar to call class method through pthread
void *measureAux(void *arg)
{
    VitalLogger *logger = (VitalLogger *)arg;
    if (logger)
    {
        logger->measureVitalSigns();
    }
    return NULL;
}

RaspAppConnection::RaspAppConnection(int listenPort, VitalLogger *loggerInstance)
{
    this->listenPort = listenPort;
    this->vitalLogger = loggerInstance;
    pthread_mutex_init(&idMutex, NULL);
}

RaspAppConnection::~RaspAppConnection()
{
    stop();
    pthread_mutex_destroy(&idMutex);
}

void RaspAppConnection::configure(std::string addr, int port)
{
    int opt = 1;

    // create socket TCP
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        return;
    }

    // config reuse addr/port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("Setsockopt failed");
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(this->listenPort);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        return;
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed");
        return;
    }

    std::cout << "Server configured on port " << this->listenPort << std::endl;
}

void RaspAppConnection::start()
{
    std::cout << "Waiting for commands" << std::endl;
    int addrlen = sizeof(address);

    while (true)
    {
        // blocks waiting for connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            break;
        }

        std::string request = receiveData();
        if (request.empty())
        {
            close(new_socket);
            continue;
        }

        // process the command
        processRequest(request);

        // build and send the response
        std::string response = buildResponse(request);
        sendData(response);

        close(new_socket);
    }
}

void RaspAppConnection::stop()
{
    if (server_fd > 0)
    {
        close(server_fd);
        server_fd = -1;
    }
}

std::string RaspAppConnection::receiveData()
{
    char buffer[BUFFER_SIZE] = {0};
    std::string request = "";

    ssize_t bytesRead = read(new_socket, buffer, BUFFER_SIZE - 1);
    if (bytesRead <= 0)
        return "";

    request.append(buffer, bytesRead);

    if (request.find("POST") != std::string::npos)
    {

        size_t headerEnd = request.find("\r\n\r\n");
        size_t clPos = request.find("Content-Length:");

        if (headerEnd != std::string::npos && clPos != std::string::npos)
        {

            size_t valStart = clPos + 15;
            size_t valEnd = request.find("\r\n", valStart);

            int contentLen = 0;
            try
            {
                std::string lenStr = request.substr(valStart, valEnd - valStart);
                contentLen = std::stoi(lenStr);
            }
            catch (...)
            {
                contentLen = 0;
            }

            size_t bodyStart = headerEnd + 4;
            size_t totalReceived = request.length();
            size_t bodyRead = 0;

            if (totalReceived > bodyStart)
            {
                bodyRead = totalReceived - bodyStart;
            }

            while (bodyRead < contentLen)
            {
                memset(buffer, 0, BUFFER_SIZE);

                bytesRead = read(new_socket, buffer, BUFFER_SIZE - 1);

                if (bytesRead <= 0)
                    break; // Conexão caiu

                request.append(buffer, bytesRead);
                bodyRead += bytesRead;
            }
        }
    }

    return request;
}

void RaspAppConnection::processRequest(std::string request)
{
    // extract userID from JSON
    size_t idPos = request.find("\"userId\":");
    if (idPos != std::string::npos)
    {
        size_t startQuote = request.find("\"", idPos + 9);
        size_t endQuote = request.find("\"", startQuote + 1);

        if (startQuote != std::string::npos && endQuote != std::string::npos)
        {
            std::string tempId = request.substr(startQuote + 1, endQuote - startQuote - 1);
            pthread_mutex_lock(&idMutex);
            this->currentUserId = tempId; // write
            pthread_mutex_unlock(&idMutex);
            std::cout << "Target User ID: " << this->currentUserId << std::endl;
        }
    }

    // process commands
    if (request.find("START_AUTH") != std::string::npos)
    {

        if (this->vitalLogger != nullptr)
        {
            // pass the extracted userID
            this->vitalLogger->startAuthentication(this->currentUserId);
        }
        else
        {
            std::cerr << "Error" << std::endl;
        }
    }
    else if (request.find("START_MEASURE") != std::string::npos)
    {

        if (this->vitalLogger != nullptr)
        {
            pthread_t measureThread;
            pthread_attr_t attr;

            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

            // create thread using the auxiliar method
            if (pthread_create(&measureThread, &attr, measureAux, (void *)this->vitalLogger) != 0)
            {
                std::cerr << "Failed to start measure thread" << std::endl;
            }

            pthread_attr_destroy(&attr);
        }
        else
        {
            std::cerr << "Error" << std::endl;
        }
    }
}

std::string RaspAppConnection::buildResponse(std::string request)
{
    // response for CORS (Preflight)
    if (request.find("OPTIONS") != std::string::npos)
    {
        return "HTTP/1.1 200 OK\r\n"
               "Access-Control-Allow-Origin: *\r\n"
               "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
               "Access-Control-Allow-Headers: Content-Type\r\n"
               "Content-Length: 0\r\n\r\n";
    }

    // default response (JSON)
    return "HTTP/1.1 200 OK\r\n"
           "Access-Control-Allow-Origin: *\r\n"
           "Content-Type: application/json\r\n\r\n"
           "{\"status\":\"success\", \"message\":\"Command Executed\"}";
}

void RaspAppConnection::sendData(std::string data)
{
    if (new_socket > 0)
    {
        ssize_t sent_bytes = send(new_socket, data.c_str(), data.length(), MSG_NOSIGNAL);
    }
}

std::string RaspAppConnection::getCurrentUserId()
{
    std::string tempId;

    pthread_mutex_lock(&idMutex);
    tempId = this->currentUserId;
    pthread_mutex_unlock(&idMutex);

    return tempId;
}