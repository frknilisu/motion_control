#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include "Init.h"
#include "AS5600.h"

#define ENCODER_RESOLUTION  12

#define SDA 21
#define SCL 22

class EncoderManager {
  public:
    EncoderManager();
    void runLoop();
  private:
    AMS_5600 ams5600;
    int previousAbsoluteStep = 0;
    int currentAbsoluteStep = 0;
    int roundCounter = 0;

    EncoderData_t encoderData;
    
    void init();
    void checkSegment();
};

#endif
