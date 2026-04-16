#include "raspCloudConnection.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#define FIREBASE_URL "https://vitallogger-ae687-default-rtdb.firebaseio.com"
#define AUTH_SECRET "2sdC0GFLPN0U8KGBiyp8hurpzP3ZxWrXA8xmAfsO"

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    return size * nmemb;
}

RaspCloudConnection::RaspCloudConnection(std::string apiUrl, std::string authToken)
    : apiUrl(apiUrl), authToken(authToken)
{
    curl_global_init(CURL_GLOBAL_ALL);
}

RaspCloudConnection::~RaspCloudConnection()
{
    curl_global_cleanup();
}

void RaspCloudConnection::performCurlRequest(std::string url, std::string jsonPayload, std::string httpMethod)
{
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        if (httpMethod == "POST")
        {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        }
        else if (httpMethod == "PATCH")
        {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        }
        else if (httpMethod == "DELETE")
        {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "Cloud error " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

void RaspCloudConnection::insertMeasurement(std::string userId, int bpm, float spo2, long timestamp)
{
    std::stringstream urlSs, jsonSs;
    urlSs << this->apiUrl << "/measurements/" << userId << ".json?auth=" << this->authToken;
    jsonSs << "{\"bpm\": " << bpm << ", \"spo2\": " << spo2 << ", \"timestamp\": " << timestamp << "}";
    performCurlRequest(urlSs.str(), jsonSs.str(), "POST");
}

void RaspCloudConnection::updateLiveStatus(std::string userId, int bpm, float spo2)
{
    std::stringstream urlSs, jsonSs;
    urlSs << this->apiUrl << "/measurements/" << userId << "/live.json?auth=" << this->authToken;
    jsonSs << "{\"bpm\": " << bpm << ", \"spo2\": " << spo2 << "}";
    performCurlRequest(urlSs.str(), jsonSs.str(), "PATCH");
}

void RaspCloudConnection::clearLiveStatus(std::string userId)
{
    std::stringstream urlSs;
    urlSs << this->apiUrl << "/measurements/" << userId << "/live.json?auth=" << this->authToken;
    performCurlRequest(urlSs.str(), "", "DELETE");
}
