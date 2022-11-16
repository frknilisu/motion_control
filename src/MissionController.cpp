#include "MissionController.h"

StaticJsonDocument<1024> actionDataJson;
static const char* TAG = "MissionController";

//esp_log_level_set("*", ESP_LOG_ERROR);        // set all components to ERROR level
//esp_log_level_set("wifi", ESP_LOG_WARN);      // enable WARN logs from WiFi stack
//esp_log_level_set("MissionController", ESP_LOG_INFO);     // enable INFO logs from DHCP client


#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"


MissionController::MissionController() :
    stateManual([this]() { manual_enter(); },
            [this]() { manual_on(); },
            [this]() { manual_exit(); }),
            
    stateProgramming([this]() { programming_enter(); },
            [this]() { programming_on(); },
            [this]() { programming_exit(); }),

    stateAction([this]() { action_enter(); },
            [this]() { action_on(); },
            [this]() { action_exit(); }),
    
    fsm(&stateManual),
    
    currentState(StateEnum::ManualState) {
  //Serial.println(">>>>>>>> MissionController() >>>>>>>>");
  ESP_LOGI(TAG, "");
}

void MissionController::init() {
  //Serial.println(">>>>>>>> MissionController::init() >>>>>>>>");
  ESP_LOGI(TAG, "");

  qMissionTask = xQueueCreate(1, sizeof(StaticJsonDocument<256>));

  auto onTimer = [](xTimerHandle pxTimer){ 
    MissionController* mc = static_cast<MissionController*>(pvTimerGetTimerID(pxTimer)); // Retrieve the pointer to class
    mc->onMsgReceived();
  };
  
  this->timerHandle = xTimerCreate(
      "timer1Sec", /* name */
      pdMS_TO_TICKS(1000), /* period/time */
      pdTRUE, /* auto reload */
      static_cast<void*>(this), /* timer ID */
      onTimer); /* callback */
  
  xTimerStart(this->timerHandle, 0);

  fsm.add_transition(&stateManual, &stateProgramming, START_PROGRAMMING_EVENT, nullptr );
  fsm.add_transition(&stateProgramming, &stateAction, FINISH_PROGRAMMING_EVENT, nullptr );
  fsm.add_transition(&stateAction, &stateManual, FINISH_ACTION_EVENT, nullptr );

  esp_log_level_set("MissionController", ESP_LOG_DEBUG);
}

void MissionController::runLoop() {
  for(;;) {
    fsm.run_machine();
    vTaskDelay(1000);
  }
}

/*--------------------------------------------------------------*/
/*---------------------- Utility Functions ---------------------*/
/*--------------------------------------------------------------*/

void MissionController::setA() {
  pa = lastMotorPosition;
  Serial.println(pa);
  isSetA = true;
}

void MissionController::setB() {
  pb = lastMotorPosition;
  Serial.println(pb);
  isSetB = true;
}

void MissionController::setActionData() {
  txJsonDoc.clear();

  // Add Motor Data
  actionDataJson["data"]["motor"]["pa"] = pa;
  actionDataJson["data"]["motor"]["pb"] = pb;
  isSetActionData = true;
}

void MissionController::setStartProgramming() {
  isStartProgramming = true;
}

void MissionController::setFinishProgramming() {
  isFinishProgramming = true;
}

void MissionController::onMsgReceived() {
  if(uxQueueMessagesWaiting(qMissionTask) != 0) {
    xReturn = xQueueReceive(qMissionTask, &rxJsonDoc, 0);
    if(xReturn == pdTRUE) {
      isNewMsgReceived = true;
      if(rxJsonDoc["msg"] == "motorPosition") {
        lastMotorPosition = rxJsonDoc["data"];
      } else if(rxJsonDoc["cmd"] == "SET_ACTION_DATA_CMD") {
        Serial.println("set action data");
        actionDataJson.clear();
        actionDataJson = rxJsonDoc;
      } else {
        Serial.println("ELSE");
      }
    } else {
      isNewMsgReceived = false;
    }
  }
}

/*--------------------------------------------------------------*/
/*---------------------- State Functions -----------------------*/
/*--------------------------------------------------------------*/

void MissionController::manual_enter() {
  ESP_LOGD(TAG, "");
  currentState = StateEnum::ManualState;
}

void MissionController::manual_on() {
  ESP_LOGV(TAG, "--- Update: MissionController -> MANUAL ---");
  
  if(!isNewMsgReceived) return;

  const char* cmd = rxJsonDoc["cmd"];
  if(cmd == "START_PROGRAMMING_CMD" && !isStartProgramming) {
    isStartProgramming = true;
    fsm.trigger(START_PROGRAMMING_EVENT);
  }

  isNewMsgReceived = false;
  rxJsonDoc.clear();
}

void MissionController::manual_exit() {
  ESP_LOGD(TAG, "");
}

void MissionController::programming_enter() {
  ESP_LOGI(TAG, "--- Enter: MissionController -> PROGRAMMING ---");
  currentState = StateEnum::ProgrammingState;
}

void MissionController::programming_on() {
  ESP_LOGV(TAG, "--- Update: MissionController -> PROGRAMMING ---");

  if(!isNewMsgReceived) return;

  const char* cmd = rxJsonDoc["cmd"];
  if(cmd == "SET_A_CMD" && !isSetA && !isSetB) {
    this->setA();
    Serial.println("setA done");
    ESP_LOGI(TAG, "--- Enter: MissionController -> ACTION ---");
  } else if(cmd == "SET_B_CMD" && isSetA && !isSetB) {
    this->setB();
    Serial.println("setB done");
    ESP_LOGI(TAG, "--- Enter: MissionController -> ACTION ---");
  } else if(cmd == "SET_ACTION_DATA_CMD" && isSetA && isSetB) {
    this->setActionData();
    Serial.println("setActionData done");
    serializeJson(actionDataJson, Serial);
    ESP_LOGI(TAG, "--- Enter: MissionController -> ACTION ---");
  } else if(cmd == "FINISH_PROGRAMMING_CMD" && isSetA && isSetB && isSetActionData) {
    this->setFinishProgramming();
    ESP_LOGI(TAG, "--- Enter: MissionController -> ACTION ---");
  }

  isNewMsgReceived = false;
  rxJsonDoc.clear();

  if(isSetA && isSetB && isSetActionData && isFinishProgramming) {
    Serial.println("Point A and B are set. Finishing action programming..");
    ESP_LOGI(TAG, "--- Enter: MissionController -> ACTION ---");
    fsm.trigger(FINISH_PROGRAMMING_EVENT);
  }
}

void MissionController::programming_exit() {
  ESP_LOGI(TAG, "--- Enter: MissionController -> ACTION ---");
}

void MissionController::action_enter() {
  ESP_LOGI(TAG, "--- Enter: MissionController -> ACTION ---");
  currentState = StateEnum::ActionState;

  xQueueSend(qActionTask, &actionDataJson, eSetValueWithOverwrite);

  isSetA = false;
  isSetB = false;
  isSetActionData = false;
  isStartProgramming = false;
  isFinishProgramming = false;
}

void MissionController::action_on() {
  ESP_LOGV(TAG, "--- Update: MissionController -> ACTION ---");

  if(!isNewMsgReceived) return;

  const char* cmd = rxJsonDoc["cmd"];
  if(cmd == "ACTION_FINISH_MSG") {
    Serial.println("ACTION finished");
    fsm.trigger(FINISH_ACTION_EVENT);
  }
  
  isNewMsgReceived = false;
  rxJsonDoc.clear();
}

void MissionController::action_exit() {
  ESP_LOGI(TAG, "--- Exit: MissionController -> ACTION ---");
}
