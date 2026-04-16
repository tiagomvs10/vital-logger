#include "vitalLogger.h"
#include "common.h"
#include <iostream>
#include <mqueue.h>
#include <fcntl.h>
#include <cstdio>
#include <ctime>

void* t_DBRegister(void* arg)
{

    VitalLogger* logger = (VitalLogger*)arg;
    RaspAppConnection* appConn = logger->appConnection;
    RaspCloudConnection* cloudConn = logger->cloudConnection;

    // config message queue
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Measurement);

    mqd_t mq3 = mq_open(MQ_RESULT_FINAL, O_CREAT | O_RDONLY, 0644, &attr);

    if (mq3 == (mqd_t)-1)
    {
        perror("Error opening MQ_RESULT_FINAL");
        return NULL;
    }

    Measurement msg;
    unsigned int prio;
    char curlCommand[1024];

    // waiting for final result
    while (!stop)
    {
        ssize_t bytes_read = mq_receive(mq3, (char *)&msg, sizeof(msg), &prio);

        if (bytes_read >= 0)
        {
            if (msg.heartRate != -1)
            {

                // generate timestamp
                std::time_t timestamp = std::time(nullptr);

                std::cout << "\n"
                          << msg.heartRate << " BPM | Timestamp: " << timestamp << std::endl;

                if (appConn != nullptr && cloudConn != nullptr)
                {
                    std::string uid = appConn->getCurrentUserId();

                    if (!uid.empty())
                    {

                        // insert final measurement
                        cloudConn->insertMeasurement(uid, msg.heartRate, msg.spo2, (long)timestamp);

                        cloudConn->clearLiveStatus(uid);
                    }
                }

                stop = 1;
                break;
            }
        }
    }
    mq_close(mq3);
    return NULL;
}


