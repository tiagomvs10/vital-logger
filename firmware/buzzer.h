#ifndef BUZZER_H
#define BUZZER_H

#include <string>
#include <signal.h>
#include <time.h>

class Buzzer
{
private:
    std::string driverPath;
    timer_t timerId;                           // POSIX timer id
    static void timerHandler(union sigval sv); // timer callback
public:
    Buzzer(const std::string &path);
    ~Buzzer();
    void turnOn(int durationMs);
    void turnOff();
};

#endif
