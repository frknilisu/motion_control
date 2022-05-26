#include "MotorManager.h"

QueueHandle_t qMotorTask;

int counter = 0;
char str[80];
bool stateChanged = false;

MotorManager::MotorManager() {
  Serial.println(">>>>>>>> MotorManager() >>>>>>>>");

  this->init();
}

void MotorManager::init() {
  Serial.println(">>>>>>>> MotorManager::init() >>>>>>>>");

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(MS0, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  
  this->stepper = AccelStepper(MotorInterfaceType, stepPin, dirPin);
  this->setStepResolution(StepType::_1_div_8);
  //this->stepper.setMaxSpeed(1000);
  //this->stepper.setAcceleration(100);
  this->stepper.setSpeed(200);
  this->stepper.moveTo(20000);

  qMotorTask = xQueueCreate(1, sizeof(int));
  if (qMotorTask == NULL) {
    Serial.println("Queue can not be created");
  }
}

void MotorManager::setStepResolution(StepType stepType) {
  /*
  M0	  M1	  M2	  Microstep Resolution
  Low	  Low	  Low	  Full step
  High	Low	  Low	  Half step
  Low	  High	Low	  1/4 step
  High	High	Low	  1/8 step
  Low	  Low	  High	1/16 step
  High	Low	  High	1/32 step
  Low	  High	High	1/32 step
  High	High	High	1/32 step
  */
  Serial.println(">>>>>>>> setStepResolution() >>>>>>>>");
  switch(stepType) {
    case StepType::Full:
      this->stepType = StepType::Full;
      digitalWrite(MS0, LOW);
      digitalWrite(MS1, LOW);
      digitalWrite(MS2, LOW);
      break;
    case StepType::Half:
      this->stepType = StepType::Half;
      digitalWrite(MS0, HIGH);
      digitalWrite(MS1, LOW);
      digitalWrite(MS2, LOW);
      break;
    case StepType::_1_div_4:
      this->stepType = StepType::_1_div_4;
      digitalWrite(MS0, LOW);
      digitalWrite(MS1, HIGH);
      digitalWrite(MS2, LOW);
      break;
    case StepType::_1_div_8:
      this->stepType = StepType::_1_div_8;
      digitalWrite(MS0, HIGH);
      digitalWrite(MS1, HIGH);
      digitalWrite(MS2, LOW);
      break;
    case StepType::_1_div_16:
      this->stepType = StepType::_1_div_16;
      digitalWrite(MS0, LOW);
      digitalWrite(MS1, LOW);
      digitalWrite(MS2, HIGH);
      break;
    case StepType::_1_div_32:
      this->stepType = StepType::_1_div_32;
      digitalWrite(MS0, HIGH);
      digitalWrite(MS1, LOW);
      digitalWrite(MS2, HIGH);
      break;
  }
}

void MotorManager::setMotorStatus(std::string stateName) {
  if(stateName == "RUN") {
    this->currentState = States::RUN;
  } else if(stateName == "STOP") {
    this->currentState = States::STOP;
  } else if(stateName == "IDLE") {
    this->currentState = States::IDLE;
  }
  stateChanged = true;
}

int MotorManager::getCurrentPosition() {
  return this->stepper.currentPosition();
}

void MotorManager::runLoop() {
  for (;;)
  {
    xReturn = xTaskNotifyWait(0, 0, &value, pdMS_TO_TICKS(10));
    if(xReturn == pdTRUE) {
      hasNewNotify = true;
      motorActionCommand = *(MotorActionCommand_t*)(value);
    } else {
      hasNewNotify = false;
    }
    switch(this->currentState) 
    {
      case States::IDLE:
        if(stateChanged) {
          Serial.println("--- Motor -> IDLE ---");
          stateChanged = false;
        }
        if(hasNewNotify) {
          if(motorActionCommand.cmd == Commands_t::MOTOR_RUN_CMD) {
            this->setMotorStatus("RUN");
          } else if(motorActionCommand.cmd == Commands_t::MOTOR_START_ACTION_CMD) {
            Serial.println("Motor Start Action");
            sprintf(str, "pA: %d, pB: %d, dir: %d", motorActionCommand.pointA, motorActionCommand.pointB, motorActionCommand.direction);
            Serial.println(str);
            stepper.moveTo(motorActionCommand.pointA);
            this->setMotorStatus("RUN");
          }
          hasNewNotify = false;
        }
        break;
      case States::RUN:
        if(stateChanged) {
          Serial.println("--- Motor -> RUN ---");
          stateChanged = false;
        }
        if(hasNewNotify) {
          if(motorActionCommand.cmd == Commands_t::MOTOR_STOP_CMD) {
            this->setMotorStatus("STOP");
          }
          hasNewNotify = false;
        } else {
          if(stepper.distanceToGo() == 0) {
            //stepper.moveTo(-stepper.currentPosition());
            Serial.println("--- Motor -> Time To Stop ---");
            this->setMotorStatus("STOP");
            missionControlCommand.cmd = Commands_t::ACTION_FINISH_MSG;
            xTaskNotify(missionControlTaskHandle, (uint32_t)&missionControlCommand, eSetValueWithOverwrite);
          } else {
            this->stepper.run();
            Serial.println("--- Motor -> MOVING ---");
            //this->stepper.runSpeedToPosition();
          }
        }
        break;
      case States::STOP:
        if(stateChanged) {
          Serial.println("--- Motor -> STOP ---");
          stateChanged = false;
        }
        this->stepper.stop();
        vTaskDelay(1000);
        this->setMotorStatus("IDLE");
        break;
    }


    currentStepPosition = this->getCurrentPosition();
    //sprintf(str, "currentPosition: %d", currentStepPosition);
    //Serial.println(str);
    xReturn = xQueueOverwrite(qMotorTask, &currentStepPosition);
    
    vTaskDelay(10);
  }
}
