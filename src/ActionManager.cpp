#include "ActionManager.h"
#include "PhotoTimelapse.h"

PhotoTimelapse* activeAction = NULL;

ActionManager::ActionManager() {
  Serial.println(">>>>>>>> ActionManager() >>>>>>>>");
}

void ActionManager::init() {
  Serial.println(">>>>>>>> ActionManager::init() >>>>>>>>");

  qActionTask = xQueueCreate(1, sizeof(StaticJsonDocument<256>));

  auto onTimer = [](xTimerHandle pxTimer){ 
    ActionManager* am = static_cast<ActionManager*>(pvTimerGetTimerID(pxTimer)); // Retrieve the pointer to class
    am->onMsgReceived();
  };
  
  this->timerHandle = xTimerCreate(
      "timer1Sec", /* name */
      pdMS_TO_TICKS(1000), /* period/time */
      pdTRUE, /* auto reload */
      static_cast<void*>(this), /* timer ID */
      onTimer); /* callback */
  
  xTimerStart(this->timerHandle, 0);
}

void ActionManager::runLoop() {
  for(;;) {
    //fsm.run_machine();
    if(activeAction != NULL) {
      Serial.println("--- ActionManager: ActiveAction->run() ---");
      activeAction->run();
    }
    vTaskDelay(1000);
  }
}


void ActionManager::onMsgReceived() {
  if(uxQueueMessagesWaiting(qActionTask) != 0) {
    xReturn = xQueueReceive(qActionTask, &rxJsonDoc, 0);
    if(xReturn == pdTRUE) {
      isNewMsgReceived = true;
      activeAction = new PhotoTimelapse(rxJsonDoc);
    } else {
      isNewMsgReceived = false;
    }
  }
}

/*
txJsonDoc["target"] = "MissionController";
txJsonDoc["cmd"] = "ACTION_FINISH_MSG";
xQueueSend(qMissionTask, &txJsonDoc, eSetValueWithOverwrite);
*/