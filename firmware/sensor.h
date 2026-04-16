#ifndef SENSOR_H
#define SENSOR_H

#include <cstdint>
#include <vector>
#include "common.h"

// register addresses
#define REG_INTR_STATUS_1 0x00
#define REG_INTR_ENABLE_1 0x02
#define REG_FIFO_WR_PTR 0x04
#define REG_FIFO_OVF_CNT 0x05
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_DATA 0x07
#define REG_FIFO_CONFIG 0x08
#define REG_MODE_CONFIG 0x09
#define REG_SPO2_CONFIG 0x0A
#define REG_LED1_PA 0x0C
#define REG_LED2_PA 0x0D

class Sensor
{
private:
    int i2cFile;
    int gpioFd;
    int gpioPin; // interrupt pin
    uint8_t i2cAddress;

    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    void readBurst(uint8_t reg, uint8_t *buffer, int length);
    void setupGpioInterrupt();

public:
    Sensor(uint8_t address, int interruptPin);
    ~Sensor();
    void initSensor();
    bool waitForInterrupt(int timeout_ms = -1);
    std::vector<RawMeasurement> readFifo();
    uint8_t readPartID();
    void stopSensor();
};

#endif