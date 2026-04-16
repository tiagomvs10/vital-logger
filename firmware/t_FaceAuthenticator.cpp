#include "vitalLogger.h"
#include "common.h"
#include <iostream>
#include <unistd.h>
#include <curl/curl.h>
#include <string>
#include <sys/stat.h>
#include <cstring>

const std::string FIREBASE_DB_URL = "https://vitallogger-ae687-default-rtdb.firebaseio.com";
const std::string DB_SECRET = "2sdC0GFLPN0U8KGBiyp8hurpzP3ZxWrXA8xmAfsO";
const std::string STORAGE_BUCKET = "vitallogger-ae687.firebasestorage.app";

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return fwrite(ptr, size, nmemb, stream);
}
size_t write_dummy(void *contents, size_t size, size_t nmemb, void *userp)
{
    return size * nmemb;
}

// download (firebase -> rasp)
bool downloadFile(std::string url, std::string outputFile)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;
    bool success = false;
    curl = curl_easy_init();
    if (curl)
    {
        fp = fopen(outputFile.c_str(), "wb");
        if (fp)
        {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
            res = curl_easy_perform(curl);
            if (res == CURLE_OK)
                success = true;
            fclose(fp);
        }
        curl_easy_cleanup(curl);
    }
    return success;
}

// upload (rasp -> firebase) ---
bool uploadFile(std::string localPath, std::string remoteName)
{
    CURL *curl;
    CURLcode res;
    struct stat file_info;
    FILE *fd;
    bool success = false;

    if (stat(localPath.c_str(), &file_info) != 0)
    {
        std::cerr << "File does not exist" << localPath << std::endl;
        return false;
    }

    fd = fopen(localPath.c_str(), "rb");
    if (!fd)
    {
        std::cerr << "Error opening file" << std::endl;
        return false;
    }

    // build URL
    std::string url = "https://firebasestorage.googleapis.com/v0/b/" + STORAGE_BUCKET + "/o?name=" + remoteName;

    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *headers = NULL;
        // define what is an image
        headers = curl_slist_append(headers, "Content-Type: image/jpeg");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_READDATA, fd);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)file_info.st_size);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_dummy);

        res = curl_easy_perform(curl);

        // check HTTP response code (sucess - 200)
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (res == CURLE_OK && response_code == 200)
        {
            success = true;
            std::cout << "Upload sucess" << std::endl;
        }
        else
        {
            std::cerr << "Upload error" << curl_easy_strerror(res)
                      << " | HTTP Code: " << response_code << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    fclose(fd);
    return success;
}

// update authentication state in database
void updateFirebaseStatus(std::string userId, bool isAuthenticated, std::string photoUrl)
{
    CURL *curl;
    CURLcode res;

    std::string statusStr = isAuthenticated ? "authenticated" : "denied";

    // JSON includes the photo sent
    std::string jsonPayload = "{";
    jsonPayload += "\"auth_status\": \"" + statusStr + "\",";
    jsonPayload += "\"last_photo\": \"" + photoUrl + "\","; 
    jsonPayload += "\"timestamp\": {\".sv\": \"timestamp\"}";
    jsonPayload += "}";

    std::string url = FIREBASE_DB_URL + "/users/" + userId + "/current_session.json?auth=" + DB_SECRET;

    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_dummy);

        res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

void *t_FaceAuthenticator(void *arg)
{
    VitalLogger *logger = (VitalLogger *)arg;
    Camera *cam = &logger->cam;
    FaceVerifier *verifier = &logger->faceVerifier;
    Buzzer *buzzer = &logger->buzzer;

    std::string userId = "";
    if (logger->appConnection != nullptr)
    {
        userId = logger->appConnection->getCurrentUserId();
    }

    if (userId.empty())
    {
        std::cerr << "Auth Error: No User ID provided." << std::endl;
        logger->releaseSystem();
        return NULL;
    }

    std::string firebaseRefURL = "https://firebasestorage.googleapis.com/v0/b/" + STORAGE_BUCKET + "/o/profilePics%2F" + userId + ".jpg?alt=media";
    std::string refImageLocal = "temp_ref_" + userId + ".jpg";
    std::string tempCamImage = "temp_cam_" + userId + ".jpg";

    // photo's path in Cloud: auth_attempts/USERID_latest.jpg
    std::string uploadRemoteName = "auth_attempts%2F" + userId + "_latest.jpg";

    // capture photo
    cv::Mat frame = cam->captureImage();
    if (frame.empty())
    {
        std::cerr << "Camera error" << std::endl;
        updateFirebaseStatus(userId, false, "null");
        buzzer->turnOn(2500);
        logger->releaseSystem();
        return NULL;
    }
    cv::imwrite(tempCamImage, frame);

    // upload image
    uploadFile(tempCamImage, uploadRemoteName);

    std::string publicPhotoUrl = "https://firebasestorage.googleapis.com/v0/b/" + STORAGE_BUCKET + "/o/" + uploadRemoteName + "?alt=media";

    // download profile
    if (!downloadFile(firebaseRefURL, refImageLocal))
    {
        unlink(tempCamImage.c_str());

        // warn authenticationerror to Database, but still send the photo to see the intruder
        updateFirebaseStatus(userId, false, publicPhotoUrl);
        buzzer->turnOn(2500);
        logger->releaseSystem();
        return NULL;
    }

    bool success = false;
    try
    {
        success = verifier->areSamePerson(refImageLocal, tempCamImage);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    unlink(refImageLocal.c_str());
    unlink(tempCamImage.c_str());

    if (success)
    {
        logger->setAuthenticated(true);
        updateFirebaseStatus(userId, true, publicPhotoUrl);
    }
    else
    {
        logger->setAuthenticated(false);
        updateFirebaseStatus(userId, false, publicPhotoUrl);
        buzzer->turnOn(2500);
    }

    logger->releaseSystem();
    return NULL;
}
