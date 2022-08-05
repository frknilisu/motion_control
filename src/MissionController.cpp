#include "MissionController.h"

#define START_PROGRAMMING_EVENT 0
#define FINISH_PROGRAMMING_EVENT 1
#define FINISH_ACTION_EVENT 2

bool isNewMessageExist = false;
bool isStartProgramming = false;
bool isFinishProgramming = false;
bool isSetA = false;
bool isSetB = false;
uint32_t value;
int pa, pb, lastMotorPosition;

//StaticJsonBuffer<200> jsonBuffer;
StaticJsonDocument<200> txJsonDoc, rxJsonDoc;

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
    
    fsm(&stateManual) {
  Serial.println(">>>>>>>> MissionController() >>>>>>>>");
}

template<typename T, typename R>
void* void_cast(R(T::*f)())
{
    union
    {
        R(T::*pf)();
        void* p;
    };
    pf = f;
    return p;
}

void MissionController::init() {
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

void MissionController::manual_enter() {
  Serial.println("--- Enter: MissionController -> MANUAL ---");
}

void MissionController::manual_on() {
  Serial.println("--- Update MissionController -> MANUAL ---");

  if(isNewMessageExist && !isStartProgramming && rxJsonDoc["cmd"] == "START_PROGRAMMING_CMD") {
    isStartProgramming = true;
    fsm.trigger(START_PROGRAMMING_EVENT);
    isNewMessageExist = false;
  }
}

void MissionController::manual_exit() {
  Serial.println("--- Exit: MissionController -> MANUAL ---");
}

void MissionController::programming_enter() {
  Serial.println("--- Enter: MissionController -> PROGRAMMING ---");
}

void MissionController::programming_on() {
  Serial.println("--- Update: MissionController -> PROGRAMMING ---");

  if(isNewMessageExist) {
    Serial.println("have new notify, let's check");
    if(!isSetA && !isSetB && rxJsonDoc["cmd"] == "SET_A_CMD") {
      this->setA();
      Serial.println("setA done");
    } else if(isSetA && !isSetB && rxJsonDoc["cmd"] == "SET_B_CMD") {
      this->setB();
      Serial.println("setB done");
    } else if(isSetA && isSetB && rxJsonDoc["cmd"] == "FINISH_PROGRAMMING_CMD") {
      this->setFinishProgramming();
    }
    isNewMessageExist = false;
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

  txJsonDoc["cmd"] = "MOTOR_START_ACTION_CMD";
  txJsonDoc["start"] = -1;
  txJsonDoc["end"] = -1;
  txJsonDoc["direction"] = "a2b";

  xQueueSend(qMotorTask, &txJsonDoc, eSetValueWithOverwrite);

  isSetA = false;
  isSetB = false;
  isFinishProgramming = false;
}

void MissionController::action_on() {
  Serial.println("--- Update: MissionController -> ACTION ---");

  if(isNewMessageExist) {
    if(rxJsonDoc["cmd"] == "ACTION_FINISH_MSG") {
      fsm.trigger(FINISH_ACTION_EVENT);
    }
    isNewMessageExist = false;
  }
}

void MissionController::action_exit() {
  Serial.println("--- Exit: MissionController -> ACTION ---");
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