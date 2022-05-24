#include "MissionController.h"

//EncoderData_t pA, pB;
MotorPositionData_t pA, pB;
bool isStartProgramming = false;
bool isFinishProgramming = false;
bool isSetA = false;
bool isSetB = false;

MissionController::MissionController() {
  Serial.println(">>>>>>>> MissionController() >>>>>>>>");
}

void MissionController::setA() {
  //pA = receiveEncoderData();
  pA = receiveMotorPosition();
  isSetA = true;
}

void MissionController::setB() {
  //pB = receiveEncoderData();
  pB = receiveMotorPosition();
  isSetB = true;
}

void MissionController::setStartProgramming() {
  isStartProgramming = true;
}

void MissionController::setFinishProgramming() {
  isFinishProgramming = true;
}

MotorPositionData_t MissionController::receiveMotorPosition() {
  xReturn = xQueueReceive(qMotorTask, &value, portMAX_DELAY);
  return (*(MotorPositionData_t*)(value));
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
        if(hasNewNotify && missionControlCommand.cmd == Commands_t::START_PROGRAMMING_CMD) {
          isStartProgramming = true;
          this->currentState = States::PROGRAMMING;
          hasNewNotify = false;
        }
        break;
      case States::PROGRAMMING:
        Serial.println("--- MC -> PROGRAMMING ---");
        if(hasNewNotify) {
          if(!isSetA && !isSetB && missionControlCommand.cmd == Commands_t::SET_A_CMD) {
            this->setA();
          } else if(isSetA && !isSetB && missionControlCommand.cmd == Commands_t::SET_B_CMD) {
            this->setB();
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
        motorActionCommand.cmd = Commands_t::MOTOR_START_ACTION_CMD;
        motorActionCommand.pointA = pA;
        motorActionCommand.pointB = pB;
        motorActionCommand.direction = 1;
        xTaskNotify(motorTaskHandle, (uint32_t)&motorActionCommand, eSetValueWithOverwrite);
        xReturn = xTaskNotifyWait(0, 0, NULL, portMAX_DELAY); // finished
        this->currentState = States::PROGRAMMING;
        break;
      case States::ERROR:
        Serial.println("Error Occured");
        break;
    }
    vTaskDelay(1000);
  }
}
