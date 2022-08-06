#include "MissionController.h"

MissionController::MissionController() :
    stateManual([this]() { manual_enter(); },
            [this]() { manual_on(); },
            [this]() { manual_exit(); }),
            
    stateProgramming([this]() { programming_enter(); },
            [this]() { programming_on(); },
            [this]() { programming_exit(); }),

    stateAction([this]() { action_enter(); },
            [this]() { action_on(); },
            [this]() { action_exit(); }),
    
    fsm(&stateManual),
    currentState(StateEnum::ManualState) {
  Serial.println(">>>>>>>> MissionController() >>>>>>>>");
}

void MissionController::init() {
  qMissionControlTask = xQueueCreate(1, sizeof(StaticJsonDocument<200>));
  if (qMissionControlTask == NULL) {
    Serial.println("Queue can not be created");
  }

  auto onTimer = [](xTimerHandle pxTimer){ 
    MissionController* mc = static_cast<MissionController*>(pvTimerGetTimerID(pxTimer)); // Retrieve the pointer to class
    mc->onValueUpdate();
  };
  
  this->timerHndl1Sec = xTimerCreate(
      "timer1Sec", /* name */
      pdMS_TO_TICKS(1000), /* period/time */
      pdTRUE, /* auto reload */
      static_cast<void*>(this), /* timer ID */
      onTimer); /* callback */
  xTimerStart(this->timerHndl1Sec, 0);

  fsm.add_transition(&stateManual, &stateProgramming, START_PROGRAMMING_EVENT, nullptr );
  fsm.add_transition(&stateProgramming, &stateAction, FINISH_PROGRAMMING_EVENT, nullptr );
  fsm.add_transition(&stateAction, &stateManual, FINISH_ACTION_EVENT, nullptr );
}

void MissionController::runLoop() {
  for(;;)
  {
    fsm.run_machine();
    vTaskDelay(1000);
  }
}

/*--------------------------------------------------------------*/
/*---------------------- Utility Functions ---------------------*/
/*--------------------------------------------------------------*/

void MissionController::setA() {
  pa = lastMotorPosition;
  Serial.println(pa);
  isSetA = true;
}

void MissionController::setB() {
  pb = lastMotorPosition;
  Serial.println(pb);
  isSetB = true;
}

void MissionController::setStartProgramming() {
  isStartProgramming = true;
}

void MissionController::setFinishProgramming() {
  isFinishProgramming = true;
}

void MissionController::onValueUpdate() {
  if(uxQueueMessagesWaiting(qMissionControlTask) != 0) {
    xReturn = xQueueReceive(qMissionControlTask, &rxJsonDoc, 0);
    if(xReturn == pdTRUE) {
      isNewMessageExist = true;
      if(rxJsonDoc["msg"] == "motorPosition") {
        lastMotorPosition = rxJsonDoc["data"];
      }
    } else {
      isNewMessageExist = false;
    }
  }
}

/*--------------------------------------------------------------*/
/*---------------------- State Functions -----------------------*/
/*--------------------------------------------------------------*/

void MissionController::manual_enter() {
  Serial.println("--- Enter: MissionController -> MANUAL ---");
  currentState = StateEnum::ManualState;
}

void MissionController::manual_on() {
  //Serial.println("--- Update MissionController -> MANUAL ---");

  if(isNewMessageExist && rxJsonDoc["cmd"] == "START_PROGRAMMING_CMD" && !isStartProgramming) {
    isStartProgramming = true;
    isNewMessageExist = false;
    rxJsonDoc.clear();
    fsm.trigger(START_PROGRAMMING_EVENT);
  }
}

void MissionController::manual_exit() {
  Serial.println("--- Exit: MissionController -> MANUAL ---");
}

void MissionController::programming_enter() {
  Serial.println("--- Enter: MissionController -> PROGRAMMING ---");
  currentState = StateEnum::ProgrammingState;
}

void MissionController::programming_on() {
  //Serial.println("--- Update: MissionController -> PROGRAMMING ---");

  if(isNewMessageExist) {
    if(rxJsonDoc["cmd"] == "SET_A_CMD" && !isSetA && !isSetB) {
      this->setA();
      Serial.println("setA done");
    } else if(rxJsonDoc["cmd"] == "SET_B_CMD" && isSetA && !isSetB) {
      this->setB();
      Serial.println("setB done");
    } else if(rxJsonDoc["cmd"] == "FINISH_PROGRAMMING_CMD" && isSetA && isSetB) {
      this->setFinishProgramming();
    }
    isNewMessageExist = false;
    rxJsonDoc.clear();
  }

  if(isSetA && isSetB && isFinishProgramming) {
    Serial.println("Point A and B are set. Finishing action programming..");
    fsm.trigger(FINISH_PROGRAMMING_EVENT);
  }
}

void MissionController::programming_exit() {
  Serial.println("--- Exit: MissionController -> PROGRAMMING ---");
}

void MissionController::action_enter() {
  Serial.println("--- Enter: MissionController -> ACTION ---");
  currentState = StateEnum::ActionState;

  txJsonDoc["target"] = "MotorManager";
  txJsonDoc["cmd"] = "START_ACTION_CMD";
  txJsonDoc["start"] = pa;
  txJsonDoc["end"] = pb;
  txJsonDoc["direction"] = "a2b";

  xQueueSend(qMotorTask, &txJsonDoc, eSetValueWithOverwrite);

  isSetA = false;
  isSetB = false;
  isFinishProgramming = false;
}

void MissionController::action_on() {
  //Serial.println("--- Update: MissionController -> ACTION ---");

  if(isNewMessageExist && rxJsonDoc["cmd"] == "ACTION_FINISH_MSG") {
    Serial.println("ACTION finished");
    isNewMessageExist = false;
    rxJsonDoc.clear();
    fsm.trigger(FINISH_ACTION_EVENT);
  }
}

void MissionController::action_exit() {
  Serial.println("--- Exit: MissionController -> ACTION ---");
}
