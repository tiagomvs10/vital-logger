#include <iostream>
#include "camera.h"

Camera::Camera()
{
}

Camera::~Camera()
{
    cap.release();
}

void Camera::initializeCamera()
{
    std::string pipeline = "libcamerasrc "
                           "! video/x-raw, width=640, height=480, framerate=30/1 "
                           "! videoconvert "
                           "! appsink drop=true max-buffers=1 sync=false";

    cap.open(pipeline, cv::CAP_GSTREAMER);
}

cv::Mat Camera::captureImage()
{

    if (cap.isOpened())
    {
        for (int i = 0; i < 30; i++)
        {
            cap.grab();
        }
    }

    cv::Mat frame;
    cap.read(frame);

    if (!frame.empty())
    {
        capturedImage = frame;
    }
    else
    {
        capturedImage.release();
    }
    return capturedImage;
}

void Camera::stopCamera()
{
    if (cap.isOpened())
    {
        cap.release();
    }
}
