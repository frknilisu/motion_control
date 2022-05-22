#include "MissionController.h"

EncoderData_t pA, pB;
bool isStartProgramming = false;
bool isFinishProgramming = false;
bool isSetA = false;
bool isSetB = false;

MissionController::MissionController() {
  Serial.println(">>>>>>>> MissionController() >>>>>>>>");
}

void MissionController::setA() {
  pA = receiveEncoderData();
  isSetA = true;
}

void MissionController::setB() {
  pB = receiveEncoderData();
  isSetB = true;
}

void MissionController::setStartProgramming() {
  isStartProgramming = true;
}

void MissionController::setFinishProgramming() {
  isFinishProgramming = true;
}

EncoderData_t receiveEncoderData() {
  xStatus = xQueueReceive(qEncoderTask, &value, portMAX_DELAY);
  return (EncoderData_t)(value);
}

void MissionController::runLoop() {
  for(;;)
  {
    switch(this->currentState) {
      case States::MANUAL:
        Serial.println("--- States::MANUAL ---");
        xStatus = xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
        isStartProgramming = true;
        this->currentState = States::PROGRAMMING;
        break;
      case States::PROGRAMMING:
        Serial.println("--- States::PROGRAMMING ---");
        xStatus = xTaskNotifyWait(0, 0, &value, portMAX_DELAY);
        missionControlCommand = (MissionControlCommand_t)(value);
        if(xStatus == pdTRUE && (missionControlCommand.cmd == Commands_t::SET_A_CMD)) {
          this->setA();
        }
        xStatus = xTaskNotifyWait(0, 0, &value, portMAX_DELAY);
        missionControlCommand = (MissionControlCommand_t)(value);
        if(xStatus == pdTRUE && (missionControlCommand.cmd == Commands_t::SET_B_CMD)) {
          this->setB();
        }
        xStatus = xTaskNotifyWait(0, 0, &value, portMAX_DELAY);
        missionControlCommand = (MissionControlCommand_t)(value);
        if(xStatus == pdTRUE && (missionControlCommand.cmd == Commands_t::FINISH_PROGRAMMING_CMD)) {
          this->setFinishProgramming();
        }
        if(isSetA && isSetB && isFinishProgramming) {
          Serial.println("Point A and B are set. Finishing action programming..");
          this->currentState = States::ACTION;
        }
        break;
      case States::ACTION:
        Serial.println("--- States::ACTION ---");
        motorActionCmd.cmd = Commands_t::MOTOR_START_ACTION_CMD;
        motorActionCmd.pointA = pA;
        motorActionCmd.pointB = pB;
        motorActionCmd.direction = 1;
        xTaskNotify(motorTaskHandle, motorActionCmd, eSetValueWithOverwrite);
        xStatus = xTaskNotifyWait(0, 0, NULL, portMAX_DELAY); // finished
        this->currentState = States::PROGRAMMING;
        break;
      case States::ERROR:
        Serial.println("Error Occured");
        break;
    }
    vTaskDelay(1000);
  }
}
