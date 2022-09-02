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
  Serial.println(">>>>>>>> CaptureManager::waitTrigger() >>>>>>>>");
  xReturn = xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
  if(xReturn == pdPASS) {
    Serial.println("--- Notif received by CaptureManager ---");
  }
}