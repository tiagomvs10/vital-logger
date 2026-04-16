#ifndef FACEVERIFIER_H
#define FACEVERIFIER_H

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <iostream>
#include <vector>

class FaceVerifier
{
private:
    // pre-trained models
    cv::Ptr<cv::FaceDetectorYN> detector;
    cv::Ptr<cv::FaceRecognizerSF> recognizer;

    const float threshold = 0.4; // match when similarity >= 60% (1-threshold)
    cv::Mat detectAndAlign(const cv::Mat &image);
    std::vector<float> extractFeatures(const cv::Mat &alignedFace);
    float compareFeatures(const std::vector<float> &feat1, const std::vector<float> &feat2);

public:
    FaceVerifier(const std::string &detectorPath, const std::string &recognizerPath);
    bool areSamePerson(const std::string &img1Path, const std::string &img2Path);
};

#endif
