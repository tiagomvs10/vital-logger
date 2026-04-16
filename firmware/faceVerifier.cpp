#include "faceVerifier.h"

FaceVerifier::FaceVerifier(const std::string &detectorPath, const std::string &recognizerPath)
{
    detector = cv::FaceDetectorYN::create(detectorPath, "", cv::Size(320, 320), 0.9, 0.3, 5000);
    recognizer = cv::FaceRecognizerSF::create(recognizerPath, "");

    if (detector.empty() || recognizer.empty())
    {
        throw std::runtime_error("Failed to load models");
    }
}

cv::Mat FaceVerifier::detectAndAlign(const cv::Mat &image)
{
    cv::Mat processImg = image.clone();
    detector->setInputSize(processImg.size());

    cv::Mat faces;
    detector->detect(processImg, faces);

    if (faces.rows == 0)
    {
        std::cerr << "No faces detected" << std::endl;
        return cv::Mat();
    }

    float bestConf = 0;
    int bestIdx = 0;
    for (int i = 0; i < faces.rows; i++)
    {
        float conf = faces.at<float>(i, 14);
        if (conf > bestConf)
        {
            bestConf = conf;
            bestIdx = i;
        }
    }

    cv::Mat faceData = faces.row(bestIdx);
    cv::Mat alignedFace;
    recognizer->alignCrop(processImg, faceData, alignedFace);

    return alignedFace;
}

std::vector<float> FaceVerifier::extractFeatures(const cv::Mat &alignedFace)
{
    cv::Mat features;
    recognizer->feature(alignedFace, features);

    if (features.empty())
    {
        std::cerr << "Failed to extract features" << std::endl;
        return std::vector<float>();
    }

    std::vector<float> featureVec(features.total());
    std::memcpy(featureVec.data(), features.data, features.total() * sizeof(float));

    // L2 normalize
    float norm = 0;
    for (float val : featureVec)
    {
        norm += val * val;
    }
    norm = std::sqrt(norm);

    if (norm > 0)
    {
        for (float &val : featureVec)
        {
            val /= norm;
        }
    }

    return featureVec;
}

float FaceVerifier::compareFeatures(const std::vector<float> &feat1, const std::vector<float> &feat2)
{
    if (feat1.size() != feat2.size())
    {
        std::cerr << "Feature size mismatch" << std::endl;
        return -1;
    }

    // compute dot product
    float dotProduct = 0;
    for (size_t i = 0; i < feat1.size(); i++)
    {
        dotProduct += feat1[i] * feat2[i];
    }

    float distance = 1 - dotProduct;
    return distance;
}

bool FaceVerifier::areSamePerson(const std::string &img1Path, const std::string &img2Path)
{
    // load images
    cv::Mat img1 = cv::imread(img1Path);
    cv::Mat img2 = cv::imread(img2Path);

    if (img1.empty())
    {
        std::cerr << "Error opening profile photo (" << img1Path << ")" << std::endl;
        return false;
    }
    if (img2.empty())
    {
        std::cerr << "Error opening captured photo (" << img2Path << ")" << std::endl;
        return false;
    }

    cv::Mat face1 = detectAndAlign(img1);
    if (face1.empty())
    {
        std::cerr << ">>> No face detected on profile photo <<<" << std::endl;
        return false;
    }

    cv::Mat face2 = detectAndAlign(img2);
    if (face2.empty())
    {
        std::cerr << ">>> No face detected on captured photo <<<" << std::endl;
        return false;
    }

    std::vector<float> feat1 = extractFeatures(face1);
    std::vector<float> feat2 = extractFeatures(face2);

    if (feat1.empty() || feat2.empty())
    {
        std::cerr << "Error extracting facial features" << std::endl;
        return false;
    }

    float distance = compareFeatures(feat1, feat2);
    if (distance < 0)
        return false;

    float similarity = (1 - distance) * 100;
    std::cout << "Similarity: " << similarity << "% (Threshold: " << threshold << ")" << std::endl;

    bool match = (distance < threshold);

    if (match)
    {
        std::cout << "Face match" << std::endl;
    }
    else
    {
        std::cout << "No face match" << std::endl;
    }

    return match;
}
