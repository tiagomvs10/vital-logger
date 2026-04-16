#ifndef PPG_STRATEGY
#define PPG_STRATEGY

#include "signalStrategy.h"
#include <deque>

class PPGStrategy : public SignalStrategy
{
private:
    std::deque<int> bpmHistory;
    std::vector<double> bandPassFilter(const std::vector<double> &data);
    std::vector<double> savitzkyGolayFilter(const std::vector<double> &data);
    std::vector<int> findPeaks(const std::vector<double> &data);
    int calculateBPM(const std::vector<int> &peaks);
    float computeSpO2(const std::vector<double> &redAC, double redDC, const std::vector<double> &irAC, double irDC);

public:
    PPGStrategy();
    Measurement processAlgorithm(const std::vector<RawMeasurement> &data) override;
};

#endif