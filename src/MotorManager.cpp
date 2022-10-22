#include "MotorManager.h"

static const char* TAG = "MotorManager";

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
  
  xTimerStart(this->qMsgTimer, 0);

  fsm.add_transition(&stateIdle, &stateRun, START_RUN_EVENT, nullptr );
  fsm.add_transition(&stateRun, &stateIdle, STOP_RUN_EVENT, nullptr );
}

void MotorManager::runLoop() {
  for (;;) {
    xReturn = xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
    if(xReturn == pdPASS) {
      Serial.println("--- Notif received by MotorManager ---");
    }
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

void MotorManager::onMsgReceived() {
  if(uxQueueMessagesWaiting(qMotorTask) != 0) {
    xReturn = xQueueReceive(qMotorTask, &rxJsonDoc, 0);
    isNewMsgReceived = (xReturn == pdTRUE);
  }
}

void MotorManager::onStepRun() {
  this->stepper.runSpeed();
  this->printPosition();
  this->publishPosition();
}

/*--------------------------------------------------------------*/
/*---------------------- State Functions -----------------------*/
/*--------------------------------------------------------------*/

void MotorManager::idle_enter() {
  ESP_LOGI(TAG, "--- Enter: MotorManager -> IDLE ---");
}

void MotorManager::idle_on() {
  ESP_LOGV(TAG, "--- Update: MotorManager -> IDLE ---");

  if(!isNewMsgReceived) return;

  const char* cmd = rxJsonDoc["cmd"];
  if(cmd == "MOTOR_RUN_CMD") {
    stepper.moveTo(20000);
    fsm.trigger(START_RUN_EVENT); // RUN
  } else if(cmd == "START_ACTION_CMD") {
    Serial.println("Motor Start Action");
    int pA = rxJsonDoc["start"];
    int pB = rxJsonDoc["end"];
    //String dir = rxJsonDoc["direction"];
    sprintf(str, "pA: %d, pB: %d", pA, pB);
    Serial.println(str);
    printPosition();
    stepper.moveTo(pA);
    fsm.trigger(START_RUN_EVENT); // RUN
  } else if(cmd == "MOVE_RELATIVE") {
    int step = rxJsonDoc["step"];
    stepper.move(step);
    fsm.trigger(START_RUN_EVENT); // RUN
  } else if(cmd == "MOVE_ABSOULATE") {
    int position = rxJsonDoc["position"];
    stepper.moveTo(position);
    fsm.trigger(START_RUN_EVENT); // RUN
  } else if(cmd == "MOTOR_SET_SPEED_CMD") {
    stepper.setSpeed(rxJsonDoc["speed"]);
    fsm.trigger(START_RUN_EVENT); // RUN
  }

  isNewMsgReceived = false;
  rxJsonDoc.clear();

}

void MotorManager::idle_exit() {
  ESP_LOGI(TAG, "--- Exit: MotorManager -> IDLE ---");
}

void MotorManager::run_enter() {
  ESP_LOGI(TAG, "--- Enter: MotorManager -> RUN ---");

  xTimerStart(this->stepRunTimer, 0);
}

void MotorManager::run_on() {
  ESP_LOGV(TAG, "--- Update: MotorManager -> RUN ---");

  if(!isNewMsgReceived) return;

  const char* cmd = rxJsonDoc["cmd"];
  if(cmd == "MOTOR_STOP_CMD") {
    isNewMsgReceived = false;
    rxJsonDoc.clear();
    stopReason = "manual";
    fsm.trigger(STOP_RUN_EVENT); // STOP
    return;
  } else if(cmd == "MOTOR_SET_SPEED_CMD") {
    Serial.println("RUN manualDrive");
    stepper.setSpeed(rxJsonDoc["speed"]);
    if(rxJsonDoc["speed"] == 0) {
      stopReason = "manual";
      fsm.trigger(STOP_RUN_EVENT);
    }
  }

  if(stepper.distanceToGo() == 0) {
    Serial.println("--- Update: Motor -> DIST2GO ZERO ---");
    printPosition();
    stopReason = "auto";
    fsm.trigger(STOP_RUN_EVENT); // STOP
    return;
  }
}

void MotorManager::run_exit() {
  ESP_LOGI(TAG, "--- Exit: MotorManager -> RUN ---");

  xTimerStop(this->stepRunTimer, 0);

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
