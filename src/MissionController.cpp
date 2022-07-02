#include "MissionController.h"
#include "Fsm.h"

#define START_PROGRAMMING_EVENT 0
#define FINISH_PROGRAMMING_EVENT 1
#define FINISH_ACTION_EVENT 2

bool isNewMessageExist = false;
bool isStartProgramming = false;
bool isFinishProgramming = false;
bool isSetA = false;
bool isSetB = false;
uint32_t value;
MotorPositionData_t pA, pB;
QueueJsonMsg_t motorControlMsg;
QueueJsonMsg_t missionControlMsg;

void MissionController::manualEnterFunction() {
  Serial.println("--- Enter: MissionController -> MANUAL ---");
}

void MissionController::manualUpdateFunction() {
  Serial.println("--- Update MissionController -> MANUAL ---");

  if(isNewMessageExist && !isStartProgramming && missionControlMsg.cmd == Commands_t::START_PROGRAMMING_CMD) {
    isStartProgramming = true;
    fsm.trigger(START_PROGRAMMING_EVENT);
    isNewMessageExist = false;
  }
}

void MissionController::manualExitFunction() {
  Serial.println("--- Exit: MissionController -> MANUAL ---");
}

void MissionController::programmingEnterFunction() {
  Serial.println("--- Enter: MissionController -> PROGRAMMING ---");
}

void MissionController::programmingUpdateFunction() {
  Serial.println("--- Update: MissionController -> PROGRAMMING ---");

  if(isNewMessageExist) {
    Serial.println("have new notify, let's check");
    if(!isSetA && !isSetB && missionControlMsg.cmd == Commands_t::SET_A_CMD) {
      this->setA();
      Serial.println("setA done");
    } else if(isSetA && !isSetB && missionControlMsg.cmd == Commands_t::SET_B_CMD) {
      this->setB();
      Serial.println("setB done");
    } else if(isSetA && isSetB && missionControlMsg.cmd == Commands_t::FINISH_PROGRAMMING_CMD) {
      this->setFinishProgramming();
    }
    isNewMessageExist = false;
  }

  if(isSetA && isSetB && isFinishProgramming) {
    Serial.println("Point A and B are set. Finishing action programming..");
    fsm.trigger(FINISH_PROGRAMMING_EVENT);
  }
}

void MissionController::programmingExitFunction() {
  Serial.println("--- Exit: MissionController -> PROGRAMMING ---");
}

void MissionController::actionEnterFunction() {
  Serial.println("--- Enter: MissionController -> ACTION ---");

  motorControlMsg.cmd = Commands_t::MOTOR_START_ACTION_CMD;
  motorControlMsg.pointA = pa;
  motorControlMsg.pointB = pb;
  motorControlMsg.direction = 1;
  xTaskNotify(motorTaskHandle, (uint32_t)&motorControlMsg, eSetValueWithOverwrite);
  isSetA = false;
  isSetB = false;
  isFinishProgramming = false;
}

void MissionController::actionUpdateFunction() {
  Serial.println("--- Update: MissionController -> ACTION ---");

  if(isNewMessageExist) {
    if(missionControlMsg.cmd == Commands_t::ACTION_FINISH_MSG) {
      fsm.trigger(FINISH_ACTION_EVENT);
    }
    isNewMessageExist = false;
  }
}

void MissionController::actionExitFunction() {
  Serial.println("--- Exit: MissionController -> ACTION ---");
}

void MissionController::checkQueueForNewMessage() {
  if(uxQueueMessagesWaiting(qMissionTask) != 0) {
    xReturn = xQueueReceive(qMissionTask, &value, 0);
    if(xReturn == pdTRUE) {
      isNewMessageExist = true;
      missionControlMsg = *(MissionControlMsg_t*)(value);
    } else {
      isNewMessageExist = false;
    }
  }
}

MissionController::MissionController() {
  Serial.println(">>>>>>>> MissionController() >>>>>>>>");

  this->init();
}

void MissionController::init() {
  manualState = State(&MissionController::manualEnterFunction, &MissionController::manualUpdateFunction, &MissionController::manualExitFunction);
  programmingState = State(&MissionController::programmingEnterFunction, &MissionController::programmingUpdateFunction, &MissionController::programmingExitFunction);
  actionState = State(&MissionController::actionEnterFunction, &MissionController::actionUpdateFunction, &MissionController::actionExitFunction);

  fsm = Fsm(&manualState);

  fsm.add_transition(&manualState, &programmingState, START_PROGRAMMING_EVENT, NULL);
  fsm.add_transition(&programmingState, &actionState, FINISH_PROGRAMMING_EVENT, NULL);
  fsm.add_transition(&actionState, &manualState, FINISH_ACTION_EVENT, NULL);
}

void MissionController::setA() {
  //pA = receiveEncoderData();
  pa = receiveMotorPosition();
  Serial.println(pa);
  isSetA = true;
}

void MissionController::setB() {
  //pB = receiveEncoderData();
  pb = receiveMotorPosition();
  Serial.println(pb);
  isSetB = true;
}

void MissionController::setStartProgramming() {
  isStartProgramming = true;
}

void MissionController::setFinishProgramming() {
  isFinishProgramming = true;
}

int MissionController::receiveMotorPosition() {
  xReturn = xQueuePeek(qMissionTask, &value, 0);
  Serial.println("Queue Receive Motor Position");
  Serial.println(value);
  return value;//(*(MotorPositionData_t*)(value));
}

void MissionController::runLoop() {
  for(;;)
  {
    fsm.run_machine();
    vTaskDelay(1000);
  }
}
