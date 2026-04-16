#ifndef CAMERA_H
#define CAMERA_H

#include <opencv2/opencv.hpp>
#include <string>

class Camera
{
    cv::VideoCapture cap;
    cv::Mat capturedImage;

public:
    Camera();
    ~Camera();
    void initializeCamera();
    cv::Mat captureImage();
    void stopCamera();
};

#endif
