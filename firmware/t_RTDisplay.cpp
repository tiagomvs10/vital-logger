#include "vitalLogger.h"
#include "common.h"
#include <iostream>
#include <poll.h>
#include <mqueue.h>
#include <fcntl.h>

void *t_RTDisplay(void *arg)
{
    VitalLogger *logger = (VitalLogger *)arg;
    RaspAppConnection *appConn = logger->appConnection;
    RaspCloudConnection *cloudConn = logger->cloudConnection;

    mqd_t mq2 = (mqd_t)-1;

    while (!stop)
    {
        mq2 = mq_open(MQ_RESULT_INTERM, O_RDONLY);

        if (mq2 != (mqd_t)-1)
        {
            break;
        }
    }

    if (stop || mq2 == (mqd_t)-1)
        return NULL;

    struct pollfd fds[1];
    fds[0].fd = (int)mq2;
    fds[0].events = POLLIN;

    Measurement msg;
    unsigned int prio;
    int currentSamples = 0;

    while (!stop)
    {
        int ret = poll(fds, 1, 1000);

        if (ret > 0)
        {
            if (fds[0].revents & POLLIN)
            {
                if (mq_receive(mq2, (char *)&msg, sizeof(msg), &prio) >= 0)
                {
                    currentSamples += BATCH_SIZE;

                    if (msg.heartRate > 0)
                    {
                        std::cout << "\rBPM:" << msg.heartRate << " | SpO2:" << msg.spo2 << std::flush;

                        if (appConn != nullptr && cloudConn != nullptr)
                        {
                            std::string uid = appConn->getCurrentUserId();
                            if (!uid.empty())
                            {
                                cloudConn->updateLiveStatus(uid, msg.heartRate, msg.spo2);
                            }
                        }
                    }
                    else
                    {
                        std::cout << "\rProcessing " << currentSamples << "/800..." << std::flush;
                    }
                }
            }
        }
    }
    mq_close(mq2);
    return NULL;
}


