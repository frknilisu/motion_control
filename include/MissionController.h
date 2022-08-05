#ifndef MISSION_CONTROLLER_H
#define MISSION_CONTROLLER_H

#include "Init.h"
#include "global.h"

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
    };

    void setStartProgramming();
    void setFinishProgramming();
    void setA();
    void setB();

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
    
};

#endif
