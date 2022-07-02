#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

#include "Init.h"
#include "global.h"
#include <AccelStepper.h>
#include <string>

// Define the AccelStepper interface type
#define MotorInterfaceType AccelStepper::DRIVER
#define dirPin 26 // -> 19
#define stepPin 25 // -> 18
#define MS0 27 // -> 14
#define MS1 16 // -> 27
#define MS2 17 // -> 26
#define enablePin 12 // -> 12

class MotorManager {
  public:
    MotorManager();
    void runLoop();
  private:
    enum class States {
      IDLE,
      RUN,
      STOP
    };

    enum class StepType {
      Full,
      Half,
      _1_div_4,
      _1_div_8,
      _1_div_16,
      _1_div_32
    };

    uint32_t value;
    bool hasNewNotify = false;
    uint32_t currentStepPosition = 0;
    AccelStepper stepper;
    States currentState = States::IDLE;
    StepType stepType;

    void init();
    void setStepResolution(StepType);
    void setMotorStatus(std::string stateName);
    int getCurrentPosition();

    void checkQueueForNewMessage();
    void vTimerCallback(xTimerHandle pxTimer);

  public:
    void idleEnterFunction();
    void idleUpdateFunction();
    void idleExitFunction();
    void runEnterFunction();
    void runUpdateFunction();
    void runExitFunction();
};

#endif
