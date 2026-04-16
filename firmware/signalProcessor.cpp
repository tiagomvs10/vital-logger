#include "signalProcessor.h"
#include "ppgStrategy.h"

SignalProcessor::SignalProcessor()
{
    this->strategy = std::unique_ptr<SignalStrategy>(new PPGStrategy());
}

Measurement SignalProcessor::process(const std::vector<RawMeasurement> &fullData)
{
    if (this->strategy)
    {
        return this->strategy->processAlgorithm(fullData);
    }
    return {-1, -1};
}
