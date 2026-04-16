#include <iostream>
#include <sched.h>
#include "vitalLogger.h"

volatile sig_atomic_t stop = 0;

extern void* t_CmdParser(void* arg);

int main()
{
    try
    {
        // init hardware
        VitalLogger vitalLogger(0x57, 529);
        vitalLogger.initialize();

        // init http connection
        RaspAppConnection appConn(8080, &vitalLogger);
        appConn.configure("0.0.0.0", 8080);

        // arguments URL & token
        RaspCloudConnection cloudConn("https://vitallogger-ae687-default-rtdb.firebaseio.com",
                                      "2sdC0GFLPN0U8KGBiyp8hurpzP3ZxWrXA8xmAfsO");

        vitalLogger.setConnections(&appConn, &cloudConn);

        
        // t_CmdParser - lowest priority
        pthread_t t;
        pthread_attr_t attr;
        sched_param param;
        pthread_attr_init(&attr);
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        param.sched_priority = 10;
        pthread_attr_setschedparam(&attr, &param);

        std::cout << "Waiting for APP commands" << std::endl;

        // create thread with the APP connection address
        if (pthread_create(&t, &attr, t_CmdParser, (void*)&appConn) != 0)
        {
            std::cerr << "Failed to create thread" << std::endl;
            return 1;
        }

        pthread_attr_destroy(&attr);

        // keep main alive
        pthread_join(t, NULL);
    }
    catch (const std::exception &e)
    {
        std::cerr << "CRITICAL ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
