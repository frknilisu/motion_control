#include "MotorManager.h"

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
  this->stepper.setMaxSpeed(1000);
  this->stepper.setAcceleration(100);
  this->stepper.setSpeed(200);
  this->stepper.moveTo(20000);
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
}

int MotorManager::getCurrentPosition() {
  Serial.println(">>>>>>>> getCurrentPosition() >>>>>>>>");
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
        Serial.println("--- States::IDLE ---");
        if(hasNewNotify) {
          if(motorActionCommand.cmd == Commands_t::MOTOR_RUN_CMD) {
            this->setMotorStatus("RUN");
          } else if(motorActionCommand.cmd == Commands_t::MOTOR_START_ACTION_CMD) {
            // TODO
          }
          hasNewNotify = false;
        }
        break;
      case States::RUN:
        Serial.println("--- States::RUN ---");
        if(hasNewNotify) {
          if(motorActionCommand.cmd == Commands_t::MOTOR_STOP_CMD) {
            this->setMotorStatus("STOP");
          }
          hasNewNotify = false;
        } else {
          if(stepper.distanceToGo() == 0) {
            stepper.moveTo(-stepper.currentPosition());
          }
          this->stepper.run();
          //this->stepper.runSpeedToPosition();
        }
        break;
      case States::STOP:
        Serial.println("--- States::STOP ---");
        this->stepper.stop();
        vTaskDelay(1000);
        this->setMotorStatus("IDLE");
        break;
    }

    //Serial.println(this->getCurrentPosition());
    
    vTaskDelay(1000);
  }
}
