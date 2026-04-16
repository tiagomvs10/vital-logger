#ifndef VITALLOGGER_H
#define VITALLOGGER_H

#include "sensor.h"
#include "signalProcessor.h"
#include "camera.h"
#include "buzzer.h"
#include "faceVerifier.h"
#include "raspCloudConnection.h"
#include "raspAppConnection.h"
#include <semaphore.h>
#include <pthread.h>
#include <string>

extern void *t_FaceAuthenticator(void *arg);
extern void *t_ReadSensor(void *arg);
extern void *t_Process(void *arg);
extern void *t_RTDisplay(void *arg);
extern void *t_DBRegister(void *arg);

class VitalLogger
{
    // allow threads to access private members
    friend void *t_FaceAuthenticator(void *arg);
    friend void *t_ReadSensor(void *arg);
    friend void *t_Process(void *arg);
    friend void *t_RTDisplay(void *arg);
    friend void *t_DBRegister(void *arg);

private:
    // hardware
    Camera cam;
    Sensor sensor;
    Buzzer buzzer;

    // logic
    FaceVerifier faceVerifier;
    SignalProcessor signalProcessor;

    // connections
    RaspAppConnection *appConnection = nullptr;
    RaspCloudConnection *cloudConnection = nullptr;

    bool isAuthenticated; // facial authentication state
    sem_t systemSemaphore;
    pthread_mutex_t authMutex;

public:
    VitalLogger(uint8_t address, int interruptPin);
    ~VitalLogger();
    void setConnections(RaspAppConnection *appConn, RaspCloudConnection *cloudConn);
    void initialize();
    bool getAuthenticationStatus();
    void measureVitalSigns();
    void setAuthenticated(bool status);
    void startAuthentication(std::string userId);
    void releaseSystem();
};

#endif