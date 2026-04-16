#include "vitalLogger.h"
#include "common.h"
#include <iostream>
#include <sched.h>

VitalLogger::VitalLogger(uint8_t address, int interruptPin)
    : sensor(address, interruptPin),
      buzzer("/dev/buzzer0"),
      faceVerifier("/root/face_detection_yunet_2023mar.onnx", "/root/face_recognition_sface_2021dec.onnx")
{
    isAuthenticated = 0;

    // init semaphore as free
    sem_init(&systemSemaphore, 0, 1);

    // init mutex
    pthread_mutex_init(&authMutex, NULL);
}

VitalLogger::~VitalLogger()
{
    sem_destroy(&systemSemaphore);
    pthread_mutex_destroy(&authMutex);
}

void VitalLogger::releaseSystem()
{
    // release resource
    sem_post(&systemSemaphore);
}

void VitalLogger::initialize()
{
    try
    {
        sensor.initSensor();
        cam.initializeCamera();
    }
    catch (const std::exception &e)
    {
        std::cerr << "HW error: " << e.what() << std::endl;
    }
}

void VitalLogger::setConnections(RaspAppConnection *appConn, RaspCloudConnection *cloudConn)
{
    this->appConnection = appConn;
    this->cloudConnection = cloudConn;
}

bool VitalLogger::getAuthenticationStatus()
{
    bool statusTemp;

    pthread_mutex_lock(&authMutex);
    statusTemp = this->isAuthenticated; // read
    pthread_mutex_unlock(&authMutex);

    return statusTemp;
}

void VitalLogger::setAuthenticated(bool status)
{
    pthread_mutex_lock(&authMutex);
    this->isAuthenticated = status; // write
    pthread_mutex_unlock(&authMutex);
}

void VitalLogger::startAuthentication(std::string userId)
{
    if (sem_trywait(&systemSemaphore) != 0)
    {
        std::cout << "Busy" << std::endl;
        return;
    }

    pthread_t t_id;
    pthread_attr_t attr;
    sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    param.sched_priority = 20; // medium-low priority
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&t_id, &attr, t_FaceAuthenticator, (void *)this) != 0)
    {
        std::cerr << "Error creating auth thread" << std::endl;
        releaseSystem();
    }
    pthread_attr_destroy(&attr);
}

void VitalLogger::measureVitalSigns()
{

    if (sem_trywait(&systemSemaphore) != 0)
    {
        std::cout << "Busy" << std::endl;
        return;
    }

    stop = 0;

    sensor.initSensor();

    pthread_t t1, t2, t3, t4;
    pthread_attr_t attr;
    sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    // t_ReadSensor - highest priority
    param.sched_priority = 50;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t1, &attr, t_ReadSensor, (void *)this);

    // t_Process - medium priority
    param.sched_priority = 30;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t2, &attr, t_Process, (void *)this);

    // t_RTDisplay - medium priority
    pthread_create(&t3, &attr, t_RTDisplay, (void *)this);

    // t_DBRegister - medium-low priority
    param.sched_priority = 20;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t4, &attr, t_DBRegister, (void *)this);

    pthread_attr_destroy(&attr);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);

    sensor.stopSensor();

    mq_unlink(MQ_SENSOR_DATA);
    mq_unlink(MQ_RESULT_INTERM);
    mq_unlink(MQ_RESULT_FINAL);

    releaseSystem();
}


