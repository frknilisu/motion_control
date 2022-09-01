#include "MotorManager.h"

QueueHandle_t qMotorTask;
QueueHandle_t qMissionTask;

char str[80];

MotorManager::MotorManager() :
    stateIdle([this]() { idle_enter(); },
              [this]() { idle_on(); },
              [this]() { idle_exit(); }),
            
    stateRun([this]() { run_enter(); },
             [this]() { run_on(); },
             [this]() { run_exit(); }),
    
    fsm(&stateIdle),

    currentState(StateEnum::IdleState) {
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
  //this->stepper.moveTo(20000);

  qMotorTask = xQueueCreate(1, sizeof(StaticJsonDocument<300>));
  if (qMotorTask == NULL) {
    Serial.println("Queue can not be created");
  }

  auto onTimer = [](xTimerHandle pxTimer){ 
    MotorManager* mm = static_cast<MotorManager*>(pvTimerGetTimerID(pxTimer)); // Retrieve the pointer to class
    mm->onValueUpdate();
  };

  this->timerHandle = xTimerCreate(
      "timer1Sec", /* name */
      pdMS_TO_TICKS(1000), /* period/time */
      pdTRUE, /* auto reload */
      static_cast<void*>(this), /* timer ID */
      onTimer); /* callback */
  
  xTimerStart(this->timerHandle, 0);

  fsm.add_transition(&stateIdle, &stateRun, START_RUN_EVENT, nullptr );
  fsm.add_transition(&stateRun, &stateIdle, STOP_RUN_EVENT, nullptr );
}

void MotorManager::runLoop() {
  for (;;) {
    fsm.run_machine();
    vTaskDelay(10);
  }
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

void MotorManager::onValueUpdate() {
  if(uxQueueMessagesWaiting(qMotorTask) != 0) {
    xReturn = xQueueReceive(qMotorTask, &rxJsonDoc, 0);
    if(xReturn == pdTRUE) {
      isNewMessageExist = true;
    } else {
      isNewMessageExist = false;
    }
  }

  this->publishPosition();
}

/*--------------------------------------------------------------*/
/*---------------------- State Functions -----------------------*/
/*--------------------------------------------------------------*/

void MotorManager::idle_enter() {
  Serial.println("--- Enter: Motor -> IDLE ---");
}

void MotorManager::idle_on() {
  //Serial.println("--- Update: Motor -> IDLE ---");

  if(isNewMessageExist) {
    if(rxJsonDoc["cmd"] == "MOTOR_RUN_CMD") {
      stepper.moveTo(20000);
      fsm.trigger(START_RUN_EVENT); // RUN
    } else if(rxJsonDoc["cmd"] == "START_ACTION_CMD") {
      Serial.println("Motor Start Action");
      int pA = rxJsonDoc["start"];
      int pB = rxJsonDoc["end"];
      //String dir = rxJsonDoc["direction"];
      sprintf(str, "pA: %d, pB: %d", pA, pB);
      Serial.println(str);
      printPosition();
      stepper.moveTo(pA);
      fsm.trigger(START_RUN_EVENT); // RUN
    } else if(rxJsonDoc["cmd"] == "MOVE_RELATIVE") {
      int step = rxJsonDoc["step"];
      stepper.move(step);
      fsm.trigger(START_RUN_EVENT); // RUN
    } else if(rxJsonDoc["cmd"] == "MOVE_ABSOULATE") {
      int position = rxJsonDoc["position"];
      stepper.moveTo(position);
      fsm.trigger(START_RUN_EVENT); // RUN
    }
    isNewMessageExist = false;
    rxJsonDoc.clear();
  }
}

void MotorManager::idle_exit() {
  Serial.println("--- Exit: Motor -> IDLE ---");
}

void MotorManager::run_enter() {
  Serial.println("--- Enter: Motor -> RUN ---");
}

void MotorManager::run_on() {
  //Serial.println("--- Update: Motor -> RUN ---");

  if(isNewMessageExist && rxJsonDoc["cmd"] == "MOTOR_STOP_CMD") {
    isNewMessageExist = false;
    rxJsonDoc.clear();
    stopReason = "manual";
    fsm.trigger(STOP_RUN_EVENT); // STOP
    return;
  }

  if(stepper.distanceToGo() == 0) {
    Serial.println("--- Update: Motor -> DIST2GO ZERO ---");
    printPosition();
    stopReason = "auto";
    fsm.trigger(STOP_RUN_EVENT); // STOP
    return;
  }
  
  stepper.run();

}

void MotorManager::run_exit() {
  Serial.println("--- Exit: Motor -> RUN ---");
  stepper.stop();

  if(stopReason == "manual") {
    //txJsonDoc["target"] = "MissionController";
    //txJsonDoc["cmd"] = "ACTION_FINISH_MSG";
  } else if(stopReason == "auto") {
    Serial.println("--- Notify Action Manager to action done ---");
    xReturn = xTaskNotifyGive(actionTaskHandle);
    if(xReturn == pdPASS) {
      Serial.println("--- Notif sent by MotorManager ---");
    }
  }

  vTaskDelay(1000);
}
