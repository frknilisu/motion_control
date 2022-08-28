#include "CaptureManager.h"

CaptureManager::CaptureManager() {
  Serial.println(">>>>>>>> CaptureManager() >>>>>>>>");
}

void CaptureManager::init() {
  Serial.println(">>>>>>>> CaptureManager::init() >>>>>>>>");
}

void CaptureManager::runLoop() {
  for(;;) {
    this->waitTrigger();
    Serial.println("TAKE a CAPTURE");
    vTaskDelay(1000);
  }
}

void CaptureManager::waitTrigger() {
  xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
}