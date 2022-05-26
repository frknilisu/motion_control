#include "MissionController.h"

//EncoderData_t pA, pB;
MotorPositionData_t pA, pB;
bool isStartProgramming = false;
bool isFinishProgramming = false;
bool isSetA = false;
bool isSetB = false;
bool motorCmdSendFlag = false;

int val = 0;
int pa, pb;

MissionController::MissionController() {
  Serial.println(">>>>>>>> MissionController() >>>>>>>>");
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
  //xReturn = xQueueReceive(qMotorTask, &value, portMAX_DELAY);
  //xReturn = xQueueReceive(qMotorTask, &val, 0);
  xReturn = xQueuePeek(qMotorTask, &val, 0);
  Serial.println("Queue Receive Motor Position");
  Serial.println(val);
  return val;//(*(MotorPositionData_t*)(value));
}

/*
EncoderData_t receiveEncoderData() {
  xReturn = xQueueReceive(qEncoderTask, &value, portMAX_DELAY);
  return (*(EncoderData_t*)(value));
}
*/

void MissionController::runLoop() {
  for(;;)
  {
    xReturn = xTaskNotifyWait(0, 0, &value, pdMS_TO_TICKS(10));
    if(xReturn == pdTRUE) {
      hasNewNotify = true;
      missionControlCommand = *(MissionControlCommand_t*)(value);
    } else {
      hasNewNotify = false;
    }
    switch(this->currentState) {
      case States::MANUAL:
        Serial.println("--- MC -> MANUAL ---");
        if(hasNewNotify && !isStartProgramming && missionControlCommand.cmd == Commands_t::START_PROGRAMMING_CMD) {
          isStartProgramming = true;
          this->currentState = States::PROGRAMMING;
          hasNewNotify = false;
        }
        break;
      case States::PROGRAMMING:
        Serial.println("--- MC -> PROGRAMMING ---");
        if(hasNewNotify) {
          Serial.println("have new notify, let's check");
          if(!isSetA && !isSetB && missionControlCommand.cmd == Commands_t::SET_A_CMD) {
            this->setA();
            Serial.println("setA done");
          } else if(isSetA && !isSetB && missionControlCommand.cmd == Commands_t::SET_B_CMD) {
            this->setB();
            Serial.println("setB done");
          } else if(isSetA && isSetB && missionControlCommand.cmd == Commands_t::FINISH_PROGRAMMING_CMD) {
            this->setFinishProgramming();
          }
          hasNewNotify = false;
        }
        if(isSetA && isSetB && isFinishProgramming) {
          Serial.println("Point A and B are set. Finishing action programming..");
          this->currentState = States::ACTION;
        }
        break;
      case States::ACTION:
        Serial.println("--- MC -> ACTION ---");
        if(hasNewNotify) {
          if(missionControlCommand.cmd == Commands_t::ACTION_FINISH_MSG) {
            this->currentState = States::MANUAL;
          }
          hasNewNotify = false;
        } else if(!motorCmdSendFlag) {
          motorActionCommand.cmd = Commands_t::MOTOR_START_ACTION_CMD;
          motorActionCommand.pointA = pa;
          motorActionCommand.pointB = pb;
          motorActionCommand.direction = 1;
          xTaskNotify(motorTaskHandle, (uint32_t)&motorActionCommand, eSetValueWithOverwrite);
          motorCmdSendFlag = true;
          isSetA = false;
          isSetB = false;
          isFinishProgramming = false;
        }
        break;
      case States::ERROR:
        Serial.println("Error Occured");
        break;
    }
    vTaskDelay(1000);
  }
}
