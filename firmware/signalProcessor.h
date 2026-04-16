#ifndef SIGNAL_PROCESSOR_H
#define SIGNAL_PROCESSOR_H

#include <vector>
#include <memory> //unique_ptr
#include "common.h"
#include "signalStrategy.h"

class SignalProcessor
{
private:
    std::unique_ptr<SignalStrategy> strategy; // pointer to active strategy

public:
    SignalProcessor();
    Measurement process(const std::vector<RawMeasurement> &fullData); // delegate processing to strategy
};

#endif