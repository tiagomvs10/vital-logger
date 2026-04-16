#include "sensor.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdexcept>
#include <string>
#include <poll.h>

Sensor::Sensor(uint8_t address, int interruptPin)
    : i2cAddress(address), gpioPin(interruptPin)
{
    const char *filename = "/dev/i2c-1";
    if ((i2cFile = open(filename, O_RDWR)) < 0)
        throw std::runtime_error("I2C Fail");
    if (ioctl(i2cFile, I2C_SLAVE, i2cAddress) < 0)
        throw std::runtime_error("I2C Conn Fail");
    setupGpioInterrupt();
}

Sensor::~Sensor()
{
    stopSensor();
    if (i2cFile >= 0)
        close(i2cFile);
    if (gpioFd >= 0)
        close(gpioFd);
}

void Sensor::setupGpioInterrupt()
{
    std::string pinStr = std::to_string(gpioPin);
    std::string gpioPath = "/sys/class/gpio/gpio" + pinStr;

    if (access(gpioPath.c_str(), F_OK) != 0)
    {
        int fd = open("/sys/class/gpio/export", O_WRONLY);
        if (fd > 0) // valid
        {
            write(fd, pinStr.c_str(), pinStr.length());
            close(fd);
        }
    }

    int fdDirection = open((gpioPath + "/direction").c_str(), O_WRONLY);
    if (fdDirection < 0)
    {
        sched_yield();
        fdDirection = open((gpioPath + "/direction").c_str(), O_WRONLY);
    }
    if (fdDirection >= 0) // valid
    {
        write(fdDirection, "in", 2);
        close(fdDirection);
    }

    int fdEdge = open((gpioPath + "/edge").c_str(), O_WRONLY);
    if (fdEdge >= 0)
    {
        write(fdEdge, "falling", 7);
        close(fdEdge);
    }

    gpioFd = open((gpioPath + "/value").c_str(), O_RDONLY);
    if (gpioFd < 0)
        throw std::runtime_error("GPIO Value Fail");
}

void Sensor::initSensor()
{
    writeRegister(REG_MODE_CONFIG, 0x40);
    while (readRegister(REG_MODE_CONFIG) & 0x40)
    {
    }

    writeRegister(REG_INTR_ENABLE_1, 0xC0);
    writeRegister(REG_FIFO_CONFIG, 0x1F);
    writeRegister(REG_SPO2_CONFIG, 0x27);
    writeRegister(REG_LED1_PA, 0x24);
    writeRegister(REG_LED2_PA, 0x24);
    writeRegister(REG_MODE_CONFIG, 0x03);

    writeRegister(REG_FIFO_WR_PTR, 0x00);
    writeRegister(REG_FIFO_OVF_CNT, 0x00);
    writeRegister(REG_FIFO_RD_PTR, 0x00);
    readRegister(REG_INTR_STATUS_1);
}

bool Sensor::waitForInterrupt(int timeout_ms)
{
    struct pollfd pfd;
    pfd.fd = gpioFd;
    pfd.events = POLLPRI;
    pfd.revents = 0;

    lseek(gpioFd, 0, SEEK_SET);
    char val;
    if (read(gpioFd, &val, 1) > 0 && val == '0')
        return true;

    int ret = poll(&pfd, 1, timeout_ms);
    if (ret > 0 && (pfd.revents & POLLPRI))
    {
        lseek(gpioFd, 0, SEEK_SET);
        read(gpioFd, &val, 1);
        return true;
    }
    return false;
}

std::vector<RawMeasurement> Sensor::readFifo()
{
    std::vector<RawMeasurement> data;
    uint8_t readPtr = readRegister(REG_FIFO_RD_PTR);
    uint8_t writePtr = readRegister(REG_FIFO_WR_PTR);

    int numSamples = (writePtr >= readPtr) ? (writePtr - readPtr) : ((32 - readPtr) + writePtr);

    readRegister(REG_INTR_STATUS_1);

    if (numSamples > 0)
    {
        uint8_t buffer[192];
        readBurst(REG_FIFO_DATA, buffer, numSamples * 6); // 6 bytes per sample (3 red + 3 ir)

        data.reserve(numSamples);

        for (int i = 0; i < numSamples; i++)
        {
            int idx = i * 6;

            uint32_t red = ((static_cast<uint32_t>(buffer[idx + 0]) << 16) | (static_cast<uint32_t>(buffer[idx + 1]) << 8) | buffer[idx + 2]) & 0x03FFFF;
            uint32_t ir = ((static_cast<uint32_t>(buffer[idx + 3]) << 16) | (static_cast<uint32_t>(buffer[idx + 4]) << 8) | buffer[idx + 5]) & 0x03FFFF;
            data.push_back({red, ir});
        }
    }
    return data;
}

void Sensor::writeRegister(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    write(i2cFile, buf, 2);
}

uint8_t Sensor::readRegister(uint8_t reg)
{
    write(i2cFile, &reg, 1);
    uint8_t value = 0;
    read(i2cFile, &value, 1);
    return value;
}

void Sensor::readBurst(uint8_t reg, uint8_t *buffer, int length)
{
    write(i2cFile, &reg, 1);
    read(i2cFile, buffer, length);
}

uint8_t Sensor::readPartID()
{
    return readRegister(0xFF);
}

void Sensor::stopSensor()
{
    uint8_t conf = readRegister(REG_MODE_CONFIG);
    writeRegister(REG_MODE_CONFIG, conf | 0x80);
}