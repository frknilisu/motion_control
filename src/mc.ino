#include "Init.h"
#include "global.h"
#include "BleManager.h"
#include "MotorManager.h"
#include "MissionController.h"
#include "ActionManager.h"
#include "CaptureManager.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"

volatile QueueHandle_t qMotorTask;
volatile QueueHandle_t qMissionTask;
volatile QueueHandle_t qCaptureTask;
volatile QueueHandle_t qBleTask;
volatile QueueHandle_t qActionTask;

volatile TaskHandle_t missionControlTaskHandle;
volatile TaskHandle_t bleTaskHandle;
volatile TaskHandle_t motorTaskHandle;
volatile TaskHandle_t actionTaskHandle;
volatile TaskHandle_t captureTaskHandle;

volatile BaseType_t xReturn;

BleManager* bleManager;
MotorManager* motorManager;
MissionController* missionController;
ActionManager* actionManager;
CaptureManager* captureManager;

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

void TaskCapture(void *pvParameters) {
  Serial.println("Starting Capture Manager Task...");

  captureManager->runLoop();
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
  captureManager = new CaptureManager();

  motorManager->init();
  bleManager->init();
  missionController->init();
  actionManager->init();
  captureManager->init();

  esp_log_level_set("MissionController", ESP_LOG_DEBUG);
  esp_log_level_set("MotorManager", ESP_LOG_INFO);

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
  
  xTaskCreatePinnedToCore(
    TaskCapture
    ,  "TaskCaptureManager"
    ,  4096
    ,  NULL
    ,  2
    ,  &captureTaskHandle
    ,  1);
  
}

void loop() {
  
}
