#ifndef COMMON_H
#define COMMON_H

#include <csignal>
#include <mqueue.h>
#include <cstdint>

#define MQ_SENSOR_DATA "/mq_sensor_data_prod"
#define MQ_RESULT_INTERM "/mq_result_interm_prod"
#define MQ_RESULT_FINAL "/mq_result_final_prod"

#define BATCH_SIZE 100 //100 samples per queue
#define HISTORY_SIZE 5

struct RawMeasurement
{
    uint32_t redValue;
    uint32_t irValue;
};

struct Measurement
{
    int heartRate;
    float spo2;
};

//global control
extern volatile sig_atomic_t stop;

#endif