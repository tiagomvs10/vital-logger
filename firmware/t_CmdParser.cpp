#include "raspAppConnection.h"
#include <iostream>

void* t_CmdParser(void* arg)
{
    RaspAppConnection *connection = (RaspAppConnection *)arg;

    if (connection == nullptr)
    {
        std::cerr << "No connection" << std::endl;
        return NULL;
    }

    // infinite loop listening (Accept -> Receive -> Parse)
    connection->start();

    // if start() returns, either the socket closed or there was an error
    return NULL;
}


