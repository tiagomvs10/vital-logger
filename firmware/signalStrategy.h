#ifndef SIGNAL_STRATEGY
#define SIGNAL_STRATEGY

#include <vector>
#include "common.h"

// interface
class SignalStrategy
{
public:
    virtual ~SignalStrategy() = default;
    virtual Measurement processAlgorithm(const std::vector<RawMeasurement> &data) = 0;
};

#endif