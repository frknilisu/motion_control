#include "Init.h"
#include "global.h"
#include "BleManager.h"
//#include "EncoderManager.h"
#include "MotorManager.h"
//#include "MissionController.h"

//TaskHandle_t missionControlTaskHandle;
//TaskHandle_t bleTaskHandle;
//TaskHandle_t motorTaskHandle;
//TaskHandle_t encoderTaskHandle;
//QueueHandle_t qEncoderTask;
//BaseType_t xReturn;

BleManager* bleManager;
//EncoderManager* encoderManager;
MotorManager* motorManager;
//MissionController* missionController;

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskBLE(void* pvParameters) {
  Serial.println("Starting BLE Task...");
  
  bleManager->runLoop();
}

/*
void TaskEncoder(void* pvParameters) {
  Serial.println("Starting Encoder Task...");

  encoderManager->runLoop();
}
*/

void TaskMotor(void *pvParameters) {
  Serial.println("Starting Motor Driving Task...");

  motorManager->runLoop();
}

/*
void TaskMissionControl(void *pvParameters) {
  Serial.println("Starting Mission Control Task...");

  missionController->runLoop();
}
*/

/*--------------------------------------------------*/
/*---------------------- Main ----------------------*/
/*--------------------------------------------------*/
void setup() {
  Serial.begin(115200);

  //encoderManager = new EncoderManager();
  motorManager = new MotorManager();
  bleManager = new BleManager();
  //missionController = new MissionController();

  xTaskCreatePinnedToCore(
    TaskBLE
    ,  "TaskBLE"
    ,  4096
    ,  NULL
    ,  2
    ,  &bleTaskHandle
    ,  0);

  /*
  xTaskCreatePinnedToCore(
    TaskEncoder
    ,  "TaskEncoder"
    ,  4096
    ,  NULL
    ,  2
    ,  &encoderTaskHandle
    ,  0);
  */

  xTaskCreatePinnedToCore(
    TaskMotor
    ,  "TaskMotor"
    ,  4096
    ,  NULL
    ,  2
    ,  &motorTaskHandle
    ,  1);

  /*
  xTaskCreatePinnedToCore(
    TaskMissionControl
    ,  "TaskMissionControl"
    ,  4096
    ,  NULL
    ,  2
    ,  &missionControlTaskHandle
    ,  1);
  */
  
}

void loop() {
  
}
