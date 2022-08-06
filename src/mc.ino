#include "Init.h"
#include "global.h"
#include "BleManager.h"
#include "MotorManager.h"
#include "MissionController.h"

BleManager* bleManager;
MotorManager* motorManager;
MissionController* missionController;

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

/*--------------------------------------------------*/
/*---------------------- Main ----------------------*/
/*--------------------------------------------------*/
void setup() {
  Serial.begin(115200);

  motorManager = new MotorManager();
  bleManager = new BleManager();
  missionController = new MissionController();

  motorManager->init();
  bleManager->init();
  missionController->init();

  xTaskCreatePinnedToCore(
    TaskBLE
    ,  "TaskBLE"
    ,  4096
    ,  NULL
    ,  2
    ,  &bleTaskHandle
    ,  0);

  xTaskCreatePinnedToCore(
    TaskMotor
    ,  "TaskMotor"
    ,  4096
    ,  NULL
    ,  2
    ,  &motorTaskHandle
    ,  1);

  xTaskCreatePinnedToCore(
    TaskMissionControl
    ,  "TaskMissionControl"
    ,  4096
    ,  NULL
    ,  2
    ,  &missionControlTaskHandle
    ,  1);
  
}

void loop() {
  
}
