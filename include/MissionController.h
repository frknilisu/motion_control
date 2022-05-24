#ifndef MISSION_CONTROLLER_H
#define MISSION_CONTROLLER_H

#include "Init.h"
#include "global.h"

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
    bool hasNewNotify = false;
    States currentState = States::MANUAL;
    //EncoderData_t encoderData;
    MotorActionCommand_t motorActionCommand;
    MissionControlCommand_t missionControlCommand;

    void setStartProgramming();
    void setFinishProgramming();
    void setA();
    void setB();

    MotorPositionData_t receiveMotorPosition();
};

#endif
