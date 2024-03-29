#ifndef ACTION_MANAGER_H
#define ACTION_MANAGER_H

#include "Init.h"
#include "global.h"

class ActionManager {
  public:
    ActionManager();
    void init();
    void runLoop();
  private:
    bool isNewMsgReceived = false;
    StaticJsonDocument<256> txJsonDoc, rxJsonDoc;

    void onMsgReceived();

    xTimerHandle timerHandle;
};

#endif
