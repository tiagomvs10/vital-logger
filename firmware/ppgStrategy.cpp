#include "ppgStrategy.h"
#include <algorithm> //std::sort
#include <cmath>
#include <numeric>

PPGStrategy::PPGStrategy() {}

std::vector<double> PPGStrategy::bandPassFilter(const std::vector<double> &data)
{
    if (data.empty())
        return {};
    std::vector<double> out(data.size());
    std::vector<double> hp(data.size());
    hp[0] = 0;
    for (size_t i = 1; i < data.size(); i++)
        hp[i] = 0.969 * hp[i - 1] + data[i] - data[i - 1];
    out[0] = 0;
    for (size_t i = 1; i < hp.size(); i++)
        out[i] = out[i - 1] + 0.2 * (hp[i] - out[i - 1]);
    return out;
}

std::vector<double> PPGStrategy::savitzkyGolayFilter(const std::vector<double> &data)
{
    if (data.size() < 5)
        return data;
    std::vector<double> out = data;
    for (size_t i = 2; i < data.size() - 2; i++)
    {
        out[i] = (-3 * data[i - 2] + 12 * data[i - 1] + 17 * data[i] + 12 * data[i + 1] - 3 * data[i + 2]) / 35;
    }
    return out;
}

std::vector<int> PPGStrategy::findPeaks(const std::vector<double> &data)
{

    size_t offset = 20;
    if (data.size() <= offset + 5)
        return {};

    std::vector<double> sortedData = data;
    std::sort(sortedData.begin() + offset, sortedData.end());

    // ignore 10% highest values (noise)
    size_t percentileIdx = (size_t)(sortedData.size() * 0.90);
    double robustMax = sortedData[percentileIdx];

    if (robustMax < 20)
        return {};

    double threshold = robustMax * 0.4;

    std::vector<int> peaks;
    int lastPeak = -100;

    // 25 samples = 250ms
    int minDistance = 25;

    for (size_t i = offset + 1; i < data.size() - 1; i++)
    {
        if (data[i] > threshold && data[i] > data[i - 1] && data[i] > data[i + 1])
        {
            if ((int)i - lastPeak > minDistance)
            {
                peaks.push_back(i);
                lastPeak = i;
            }
        }
    }

    return peaks;
}

int PPGStrategy::calculateBPM(const std::vector<int> &peaks)
{
    if (peaks.size() < 2)
        return -1;

    double totalInterval = 0;
    for (size_t i = 1; i < peaks.size(); i++)
        totalInterval += (peaks[i] - peaks[i - 1]);

    double avgSamples = totalInterval / (peaks.size() - 1);
    double bpm = 6000 / avgSamples;

    if (bpm < 40 || bpm > 220)
        return -1;
    return (int)bpm;
}

float PPGStrategy::computeSpO2(const std::vector<double> &redAC, double redDC, const std::vector<double> &irAC, double irDC)
{
    double redRMS = 0, irRMS = 0;
    for (double v : redAC)
        redRMS += v * v;
    for (double v : irAC)
        irRMS += v * v;
    redRMS = std::sqrt(redRMS / redAC.size());
    irRMS = std::sqrt(irRMS / irAC.size());
    if (redDC == 0 || irDC == 0 || irRMS == 0)
        return 0;
    double R = (redRMS / redDC) / (irRMS / irDC);
    double spo2 = 104 - 17 * R;
    if (spo2 > 100)
        spo2 = 100;
    if (spo2 < 60)
        spo2 = 60;
    return (float)spo2;
}

Measurement PPGStrategy::processAlgorithm(const std::vector<RawMeasurement> &fullData)
{
    Measurement result = {-1, -1};

    if (fullData.size() < 100)
        return result;

    std::vector<double> irVec, redVec;
    irVec.reserve(fullData.size());
    redVec.reserve(fullData.size());

    double irDC = 0, redDC = 0;

    for (const auto &s : fullData)
    {
        irVec.push_back(-(double)s.irValue);
        redVec.push_back(-(double)s.redValue);
        irDC += s.irValue;
        redDC += s.redValue;
    }
    irDC /= fullData.size();
    redDC /= fullData.size();

    // filter
    std::vector<double> irFiltered = savitzkyGolayFilter(bandPassFilter(irVec));
    std::vector<double> redFiltered = savitzkyGolayFilter(bandPassFilter(redVec));

    // detect peaks
    std::vector<int> peaks = findPeaks(irFiltered);

    // calculus
    int rawBPM = calculateBPM(peaks);
    float currentSpO2 = computeSpO2(redFiltered, redDC, irFiltered, irDC);

    // moving average filter for heart rate
    if (rawBPM != -1)
    {
        bpmHistory.push_back(rawBPM);
        if (bpmHistory.size() > HISTORY_SIZE)
            bpmHistory.pop_front();
    }

    if (!bpmHistory.empty())
    {
        int sum = 0;
        for (int v : bpmHistory)
            sum += v;
        result.heartRate = sum / bpmHistory.size();
    }
    else
    {
        result.heartRate = rawBPM;
    }

    result.spo2 = currentSpO2;

    return result;
}
