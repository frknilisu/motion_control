#include "Init.h"
#include "global.h"
#include "BleManager.h"
#include "MotorManager.h"
#include "MissionController.h"
#include "ActionManager.h"

TaskHandle_t missionControlTaskHandle;
TaskHandle_t bleTaskHandle;
TaskHandle_t motorTaskHandle;
TaskHandle_t actionTaskHandle;

BleManager* bleManager;
MotorManager* motorManager;
MissionController* missionController;
ActionManager* actionManager;

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskBLE(void* pvParameters) {
  Serial.println("Starting BLE Task...");
  
  bleManager->runLoop();
}

void TaskMotor(void *pvParameters) {
  Serial.println("Starting Motor Driving Task...");

  motorManager->runLoop();
}

void TaskMissionControl(void *pvParameters) {
  Serial.println("Starting Mission Control Task...");

  missionController->runLoop();
}

void TaskAction(void *pvParameters) {
  Serial.println("Starting Action Manager Task...");

  actionManager->runLoop();
}

/*--------------------------------------------------*/
/*---------------------- Main ----------------------*/
/*--------------------------------------------------*/
void setup() {
  Serial.begin(115200);

  motorManager = new MotorManager();
  bleManager = new BleManager();
  missionController = new MissionController();
  actionManager = new ActionManager();

  motorManager->init();
  bleManager->init();
  missionController->init();
  actionManager->init();

  xTaskCreatePinnedToCore(
    TaskBLE
    ,  "TaskBLE"
    ,  4096
    ,  NULL
    ,  2
    ,  &bleTaskHandle
    ,  1);

  xTaskCreatePinnedToCore(
    TaskMotor
    ,  "TaskMotor"
    ,  4096
    ,  NULL
    ,  2
    ,  &motorTaskHandle
    ,  0);

  xTaskCreatePinnedToCore(
    TaskMissionControl
    ,  "TaskMissionControl"
    ,  4096
    ,  NULL
    ,  2
    ,  &missionControlTaskHandle
    ,  1);

  xTaskCreatePinnedToCore(
    TaskAction
    ,  "TaskActionManager"
    ,  4096
    ,  NULL
    ,  2
    ,  &actionTaskHandle
    ,  1);
  
}

void loop() {
  
}
