#ifndef MISSION_CONTROLLER_H
#define MISSION_CONTROLLER_H

#include "Init.h"
#include "global.h"

#define START_PROGRAMMING_EVENT 0
#define FINISH_PROGRAMMING_EVENT 1
#define FINISH_ACTION_EVENT 2

class MissionController {
  public:
    MissionController();
    void init();
    void runLoop();
  private:
    enum class StateEnum {
      ManualState,
      ProgrammingState,
      ActionState
    } currentState;

    bool isSetA = false;
    bool isSetB = false;
    bool isSetActionData = false;
    bool isStartProgramming = false;
    bool isFinishProgramming = false;
    uint32_t value;
    int pa, pb, lastMotorPosition;

    bool isNewMessageExist = false;
    StaticJsonDocument<256> txJsonDoc, rxJsonDoc;

    void setA();
    void setB();
    void setActionData();
    void setStartProgramming();
    void setFinishProgramming();
    void onValueUpdate();

    void manual_enter();
    void manual_on();
    void manual_exit();

    void programming_enter();
    void programming_on();
    void programming_exit();

    void action_enter();
    void action_on();
    void action_exit();

    FunctionState stateManual;
    FunctionState stateProgramming;
    FunctionState stateAction;

    FunctionFsm fsm;

    xTimerHandle timerHandle;

};

#endif
