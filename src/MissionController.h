#ifndef MISSION_CONTROLLER_H
#define MISSION_CONTROLLER_H

#include "Init.h"

class MissionController {
  public:
    MissionController();
    void runLoop();
  private:
    enum class States {
      MANUAL,
      PROGRAMMING,
      ACTION,
      ERROR
    };
    
    uint32_t value;
    States currentState = States::MANUAL;
    BaseType_t xStatus;
    EncoderData_t encoderData;
    MotorActionCommand_t motorActionCommand;
    MissionControlCommand_t missionControlCommand;

    void setStartProgramming();
    void setFinishProgramming();
    void setA();
    void setB();
};

#endif
