#include "MotorManager.h"

#define START_RUN_EVENT 0
#define STOP_RUN_EVENT 1

QueueHandle_t qMotorTask;
QueueHandle_t qMissionControlTask;
JsonMsg_t rx, tx;

int counter = 0;
char str[80];
bool stateChanged = false;
bool isNewMessageExist = false;
uint32_t value;

MotorManager::MotorManager() :
    stateIdle([this]() { idle_enter(); },
              [this]() { idle_on(); },
              [this]() { idle_exit(); }),
            
    stateRun([this]() { run_enter(); },
             [this]() { run_on(); },
             [this]() { run_exit(); }),
    
    fsm(&stateIdle) {
  Serial.println(">>>>>>>> MotorManager() >>>>>>>>");
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

  qMotorTask = xQueueCreate(1, sizeof(JsonMsg_t));
  if (qMotorTask == NULL) {
    Serial.println("Queue can not be created");
  }

  xTimerHandle timerHndl1Sec;
  timerHndl1Sec = xTimerCreate(
      "timer1Sec", /* name */
      pdMS_TO_TICKS(100), /* period/time */
      pdTRUE, /* auto reload */
      (void*)0, /* timer ID */
      vTimerCallback); /* callback */
  xTimerStart(timerHndl1Sec, 0);

  fsm.add_transition(&stateIdle, &stateRun, START_RUN_EVENT, nullptr );
  fsm.add_transition(&stateRun, &stateIdle, STOP_RUN_EVENT, nullptr );
}

void MotorManager::runLoop() {
  for (;;)
  {
    fsm.run_machine();
    vTaskDelay(10);
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

void sendPose() {
  /*
  currentStepPosition = this->getCurrentPosition();
  //sprintf(str, "currentPosition: %d", currentStepPosition);
  //Serial.println(str);
  xReturn = xQueueOverwrite(qMotorTask, &currentStepPosition);
  */
}

void MotorManager::checkQueueForNewMessage() {
  if(uxQueueMessagesWaiting(qMotorTask) != 0) {
    xReturn = xQueueReceive(qMotorTask, &rx, 0);
    isNewMessageExist = xReturn == pdTRUE ? true : false;
  }
}

void MotorManager::vTimerCallback(xTimerHandle pxTimer) {
  checkQueueForNewMessage();
}

void MotorManager::idle_enter() {
  Serial.println("--- Enter: Motor -> IDLE ---");
}

void MotorManager::idle_on() {
  Serial.println("--- Update: Motor -> IDLE ---");

  if(isNewMessageExist) {
    if(motorActionCommand.cmd == Commands_t::MOTOR_RUN_CMD) {
      //fsm.trigger(runState); // RUN
    } else if(motorActionCommand.cmd == Commands_t::MOTOR_START_ACTION_CMD) {
      Serial.println("Motor Start Action");
      sprintf(str, "pA: %d, pB: %d, dir: %d", motorActionCommand.pointA, motorActionCommand.pointB, motorActionCommand.direction);
      Serial.println(str);
      stepper.moveTo(motorActionCommand.pointA);
      fsm.trigger(START_RUN_EVENT); // RUN
    }
    isNewMessageExist = false;
  }
}

void MotorManager::idle_exit() {
  Serial.println("--- Exit: Motor -> IDLE ---");
}

void MotorManager::run_enter() {
  Serial.println("--- Enter: Motor -> RUN ---");
}

void MotorManager::run_on() {
  Serial.println("--- Update: Motor -> RUN ---");

  if(isNewMessageExist) {
    if(motorActionCommand.cmd == Commands_t::MOTOR_STOP_CMD) {
      fsm.trigger(STOP_RUN_EVENT); // STOP
    }
    isNewMessageExist = false;
  }

  if(stepper.distanceToGo() == 0) {
    fsm.trigger(STOP_RUN_EVENT); // STOP
  }
  
  this->stepper.run();

}

void MotorManager::run_exit() {
  Serial.println("--- Exit: Motor -> RUN ---");
  this->stepper.stop();
  xQueueSend(qMissionControlTask, tx, 0);

  //missionControlCommand.cmd = Commands_t::ACTION_FINISH_MSG;
  //xTaskNotify(missionControlTaskHandle, (uint32_t)&missionControlCommand, eSetValueWithOverwrite);
  vTaskDelay(1000);
}