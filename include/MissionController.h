#ifndef MISSION_CONTROLLER_H
#define MISSION_CONTROLLER_H

#include "Init.h"
#include "global.h"

class MissionController {
  public:
    MissionController();
    void runLoop();
  private:
    void init();
    void setStartProgramming();
    void setFinishProgramming();
    void setA();
    void setB();

    void checkQueueForNewMessage();
    int receiveMotorPosition();

    State manualState;
    State programmingState;
    State actionState;
    Fsm fsm;

  public:
    void manualEnterFunction();
    void manualUpdateFunction();
    void manualExitFunction();

    void programmingEnterFunction();
    void programmingUpdateFunction();
    void programmingExitFunction();

    void actionEnterFunction();
    void actionUpdateFunction();
    void actionExitFunction();
};

#endif
