#include "ActionManager.h"
#include "PhotoTimelapse.h"

QueueHandle_t qActionTask;
PhotoTimelapse* activeAction = NULL;

ActionManager::ActionManager() {
  Serial.println(">>>>>>>> ActionManager() >>>>>>>>");
}

void ActionManager::init() {
  Serial.println(">>>>>>>> ActionManager::init() >>>>>>>>");

  qActionTask = xQueueCreate(1, sizeof(StaticJsonDocument<300>));
  if (qActionTask == NULL) {
    Serial.println("Queue can not be created");
  }

  auto onTimer = [](xTimerHandle pxTimer){ 
    ActionManager* am = static_cast<ActionManager*>(pvTimerGetTimerID(pxTimer)); // Retrieve the pointer to class
    am->onValueUpdate();
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


void ActionManager::onValueUpdate() {
  if(uxQueueMessagesWaiting(qActionTask) != 0) {
    xReturn = xQueueReceive(qActionTask, &rxJsonDoc, 0);
    if(xReturn == pdTRUE) {
      isNewMessageExist = true;
      activeAction = new PhotoTimelapse(rxJsonDoc);
    } else {
      isNewMessageExist = false;
    }
  }
}

/*
txJsonDoc["target"] = "MissionController";
txJsonDoc["cmd"] = "ACTION_FINISH_MSG";
xQueueSend(qMissionTask, &txJsonDoc, eSetValueWithOverwrite);
*/