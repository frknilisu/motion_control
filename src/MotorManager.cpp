#include "MotorManager.h"

static const char* TAG = "MotorManager";

char str[80];

static int zeroSpeedCounter = 0;

std::string controlType = "speed";

MotorManager::MotorManager() {
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
  this->stepper.setMaxSpeed(8000);
  //this->stepper.setAcceleration(100);
  //this->stepper.setSpeed(200);

  qMotorTask = xQueueCreate(1, sizeof(StaticJsonDocument<256>));

  auto onQMsgTimerCallback = [](xTimerHandle pxTimer){ 
    MotorManager* mm = static_cast<MotorManager*>(pvTimerGetTimerID(pxTimer)); // Retrieve the pointer to class
    mm->onMsgReceived();
  };

  auto onStepRunTimerCallback = [](xTimerHandle pxTimer){ 
    MotorManager* mm = static_cast<MotorManager*>(pvTimerGetTimerID(pxTimer)); // Retrieve the pointer to class
    mm->onStepRun();
  };

  this->qMsgTimer = xTimerCreate(
      "QueueMsgTimer", /* name */
      pdMS_TO_TICKS(300), /* period/time */
      pdTRUE, /* auto reload */
      static_cast<void*>(this), /* timer ID */
      onQMsgTimerCallback); /* callback */

  this->stepRunTimer = xTimerCreate(
      "StepRunTimer", /* name */
      pdMS_TO_TICKS(100), /* period/time */
      pdTRUE, /* auto reload */
      static_cast<void*>(this), /* timer ID */
      onStepRunTimerCallback); /* callback */
  
  //xTimerStart(this->qMsgTimer, 0);
}

void MotorManager::runLoop() {
  for (;;) {
    this->onMsgReceived();
    this->handleMsg();
    if(zeroSpeedCounter >= 5) {
      this->stepper.stop();
      zeroSpeedCounter = 0;
    }
    this->onStepRun();
    Serial.print("distanceToGo: ");
    Serial.println(this->stepper.distanceToGo());
    if(this->stepper.distanceToGo() == 0) {
      xTaskNotifyGive(actionTaskHandle);
    }
    vTaskDelay(100);
  }
}

void MotorManager::onMsgReceived() {
  if(uxQueueMessagesWaiting(qMotorTask) != 0) {
    xReturn = xQueueReceive(qMotorTask, &rxJsonDoc, 0);
    isNewMsgReceived = (xReturn == pdTRUE);
  }
}

void MotorManager::handleMsg() {
  if(!isNewMsgReceived) return;

  const char* cmd = rxJsonDoc["cmd"];
  if(cmd == "MOTOR_SET_SPEED_CMD") {
    Serial.println(">>>>>>>> MotorManager MOTOR_SET_SPEED_CMD >>>>>>>>");
    controlType = "speed";

    int speed = rxJsonDoc["speed"];
    this->stepper.setSpeed(speed);

    if(speed == 0) 
      ++zeroSpeedCounter;
  } else if(cmd == "MOTOR_MOVE_CMD") {
    Serial.println(">>>>>>>> MotorManager MOTOR_MOVE_CMD >>>>>>>>");
    controlType = "position";

    int targetPositionRelative = rxJsonDoc["relative"];
    int speed = 500;
    this->stepper.move(targetPositionRelative);
    this->stepper.setSpeed(speed);
  } else if(cmd == "MOTOR_MOVE_TO_CMD") {
    Serial.println(">>>>>>>> MotorManager MOTOR_MOVE_TO_CMD >>>>>>>>");
    controlType = "position";
    
    int targetPositionAbsolute = rxJsonDoc["absolute"];
    int speed = 500;
    this->stepper.move(targetPositionAbsolute);
    this->stepper.setSpeed(speed);
  }
  isNewMsgReceived = false;
  rxJsonDoc.clear();
}

void MotorManager::onStepRun() {
  if (controlType == "speed")
    this->stepper.runSpeed();
  else if (controlType == "position")
    this->stepper.runSpeedToPosition();
  //Serial.println(this->stepper.speed());
  //this->printPosition();
  this->publishPosition();
}


/*--------------------------------------------------------------*/
/*---------------------- Utility Functions ---------------------*/
/*--------------------------------------------------------------*/

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

int MotorManager::getCurrentPosition() {
  return this->stepper.currentPosition();
}

void MotorManager::printPosition() {
  int currPos = getCurrentPosition();
  sprintf(str, "currPos: %d", currPos);
  Serial.println(str);
}

void MotorManager::publishPosition() {
  currentStepPosition = this->getCurrentPosition();
  
  txJsonDoc["target"] = "MissionController";
  txJsonDoc["msg"] = "motorPosition";
  txJsonDoc["data"] = currentStepPosition;

  xQueueSend(qMissionTask, &txJsonDoc, eSetValueWithOverwrite);
}

