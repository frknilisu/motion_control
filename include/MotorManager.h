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
    void init();
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

    void setStepResolution(StepType);
    void setMotorStatus(std::string stateName);
    int getCurrentPosition();
    void publishPosition();

    void idle_enter();
    void idle_on();
    void idle_exit();
    void run_enter();
    void run_on();
    void run_exit();

    void vTimerCallback(xTimerHandle pxTimer);
    void onValueUpdate();

    FunctionState stateIdle;
    FunctionState stateRun;

    FunctionFsm fsm;

    xTimerHandle timerHndl1Sec;
};

#endif
