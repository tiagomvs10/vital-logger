#ifndef RASPCLOUDCONNECTION_H
#define RASPCLOUDCONNECTION_H

#include <string>
#include <curl/curl.h>

class RaspCloudConnection
{
private:
    int port;
    std::string apiUrl;    // firebase api url
    std::string authToken; // firebase auth token
    void performCurlRequest(std::string url, std::string jsonPayload, std::string httpMethod);

public:
    RaspCloudConnection(std::string apiUrl, std::string authToken);
    ~RaspCloudConnection();

    // specific firebase methods
    void insertMeasurement(std::string userId, int bpm, float spo2, long timestamp);
    void updateLiveStatus(std::string userId, int bpm, float spo2);
    void clearLiveStatus(std::string userId);
};

#endif
