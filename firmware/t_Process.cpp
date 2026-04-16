#include "vitalLogger.h"
#include <time.h>
#include <iostream>
#include <vector>
#include <mqueue.h>
#include <fcntl.h>

void *t_Process(void *arg)
{
    VitalLogger *logger = (VitalLogger *)arg;
    SignalProcessor *signalProcessor = &logger->signalProcessor;

    std::vector<RawMeasurement> accumulatedData;
    accumulatedData.reserve(800); // 800 samples total

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(RawMeasurement) * BATCH_SIZE;
    mqd_t mq1 = mq_open(MQ_SENSOR_DATA, O_CREAT | O_RDONLY, 0644, &attr);

    struct mq_attr attrRes;
    attrRes.mq_flags = 0;
    attrRes.mq_maxmsg = 10;
    attrRes.mq_msgsize = sizeof(Measurement);

    mqd_t mq2 = mq_open(MQ_RESULT_INTERM, O_CREAT | O_WRONLY, 0644, &attrRes);
    mqd_t mq3 = mq_open(MQ_RESULT_FINAL, O_CREAT | O_WRONLY, 0644, &attrRes);

    RawMeasurement bufferIn[BATCH_SIZE];

    while (!stop)
    {
        struct timespec tm;
        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_sec += 1; // 1 second timeout

        // non-blocking: if no data arrives in 1 second, loop repeats to check "stop" flag
        ssize_t bytes = mq_timedreceive(mq1, (char *)bufferIn, sizeof(RawMeasurement) * BATCH_SIZE, NULL, &tm);

        if (bytes >= 0)
        {
            for (int i = 0; i < BATCH_SIZE; i++)
            {
                accumulatedData.push_back(bufferIn[i]);
            }

            Measurement res = signalProcessor->process(accumulatedData);

            if (accumulatedData.size() < 800)
            {
                mq_send(mq2, (const char *)&res, sizeof(Measurement), 0);
            }
            else
            {
                mq_send(mq3, (const char *)&res, sizeof(Measurement), 0);
                accumulatedData.clear();
            }
        }
    }
    mq_close(mq1);
    mq_close(mq2);
    mq_close(mq3);
    return NULL;
}


