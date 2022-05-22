#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

#include "Init.h"
#include "global.h"
#include <AccelStepper.h>
#include <string>

// Define the AccelStepper interface type
#define MotorInterfaceType AccelStepper::DRIVER
#define dirPin 26
#define stepPin 25
#define MS0 27
#define MS1 16
#define MS2 17
#define enablePin 12

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
    AccelStepper stepper;
    States currentState = States::IDLE;
    StepType stepType;
    MotorActionCommand_t motorActionCommand;

    void init();
    void setStepResolution(StepType);
    void setMotorStatus(std::string stateName);
    int getCurrentPosition();
};

#endif
