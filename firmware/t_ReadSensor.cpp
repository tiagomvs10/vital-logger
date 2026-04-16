#include <iostream>
#include <vector>
#include <mqueue.h>
#include <fcntl.h>
#include <cstdio> //perror
#include "vitalLogger.h"

void *t_ReadSensor(void *arg)
{

    VitalLogger *logger = (VitalLogger *)arg;
    Sensor *sensor = &logger->sensor;

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(RawMeasurement) * BATCH_SIZE;

    mqd_t mq1 = mq_open(MQ_SENSOR_DATA, O_CREAT | O_WRONLY, 0644, &attr);
    if (mq1 == (mqd_t)-1)
    {
        perror("Error MQ1");
        return NULL;
    }

    std::vector<RawMeasurement> localBatch;
    localBatch.reserve(BATCH_SIZE);

    int warmupCounter = 0;
    const int SAMPLES_TO_IGNORE = 300;

    while (!stop)
    {
        if (sensor->waitForInterrupt(1000))
        {
            std::vector<RawMeasurement> fifoData = sensor->readFifo();

            for (const auto &s : fifoData)
            {
                if (stop)
                    break;

                if (s.irValue < 25000)
                {
                    if (!localBatch.empty())
                        localBatch.clear();
                    if (warmupCounter > 0)
                    {
                        std::cout << "\rFinger undetected" << std::flush;
                        warmupCounter = 0;
                    }
                    continue;
                }

                // discard first 300 samples (avoid noise caused by finger movement)
                if (warmupCounter < SAMPLES_TO_IGNORE)
                {
                    warmupCounter++;
                    if (warmupCounter % 100 == 0)
                        std::cout << "\rStabilizing (" << warmupCounter << "/" << SAMPLES_TO_IGNORE << ")...      " << std::flush;
                    continue;
                }

                localBatch.push_back(s);

                if (localBatch.size() == BATCH_SIZE)
                {
                    mq_send(mq1, (const char *)localBatch.data(),
                            sizeof(RawMeasurement) * BATCH_SIZE, 0);
                    localBatch.clear();
                }
            }
        }
    }
    mq_close(mq1);
    return NULL;
}




