#include "buzzer.h"
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <cstring>

Buzzer::Buzzer(const std::string &path)
{
    this->driverPath = path;
    struct sigevent sev;
    std::memset(&sev, 0, sizeof(struct sigevent));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = &Buzzer::timerHandler;
    sev.sigev_value.sival_ptr = this;

    if (timer_create(CLOCK_REALTIME, &sev, &this->timerId) == -1)
    {
        std::cerr << "error" << std::endl;
    }
}

Buzzer::~Buzzer()
{
    timer_delete(this->timerId);
    turnOff();
}

void Buzzer::turnOff()
{
    std::ofstream fileOff(this->driverPath);
    if (fileOff.is_open())
    {
        fileOff << 0;
        fileOff.close();
    }
}

void Buzzer::turnOn(int durationMs)
{
    std::ofstream fileOn(this->driverPath);
    if (fileOn.is_open())
    {
        fileOn << 1;
        fileOn.close();
    }
    else
    {
        std::cerr << "Error" << this->driverPath << std::endl;
        return;
    }

    // set timer values
    struct itimerspec its;
    long long freq_nanosecs = durationMs * 1000000LL;
    its.it_value.tv_sec = freq_nanosecs / 1000000000;
    its.it_value.tv_nsec = freq_nanosecs % 1000000000;
    its.it_interval.tv_sec = 0; // one shot
    its.it_interval.tv_nsec = 0;
    if (timer_settime(this->timerId, 0, &its, NULL) == -1)
    {
        std::cerr << "error" << std::endl;
    }
}

// called when timer overflows
void Buzzer::timerHandler(union sigval sv)
{
    // pointer to buzzer object
    Buzzer *buzzerInstance = static_cast<Buzzer *>(sv.sival_ptr);
    if (buzzerInstance)
    {
        buzzerInstance->turnOff();
    }
}
