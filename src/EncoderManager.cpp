#include "EncoderManager.h"
#include "global.h"
#include <Wire.h>

EncoderManager::EncoderManager() {
  Serial.println(">>>>>>>> EncoderManager() >>>>>>>>");

  this->init();
}

void EncoderManager::init() {
  Serial.println(">>>>>>>> EncoderManager::init() >>>>>>>>");
  
  Wire.begin(SDA, SCL);
  while(!this->ams5600.detectMagnet()) {
    delay(1000);
  }
  if(this->ams5600.detectMagnet()) {
    uint16_t magnitude = this->ams5600.getMagnitude();
    Serial.printf("Current Magnitude: %d\n", magnitude);
  } else {
    Serial.println("Can not detect magnet of Encoder");
    // xNotify ErrorCode
  }

  qEncoderTask = xQueueCreate(10, sizeof(EncoderData_t));
}

bool isCloseToPrevZone(int step) {
  return (step >= 0 && step < 1024);
}

bool isCloseToNextZone(int step) {
  return (step >= 3072 && step < 4096);
}

void EncoderManager::checkSegment() {
  Serial.println("--- checkSegment() ---");
  Serial.printf("Prev: %d, Curr: %d\n", previousAbsoluteStep, currentAbsoluteStep);
  if(isCloseToNextZone(previousAbsoluteStep) && isCloseToPrevZone(currentAbsoluteStep)) {
    ++roundCounter;
    Serial.printf("Round counter is increased: %d\n", roundCounter);
  } else if(isCloseToPrevZone(previousAbsoluteStep) && isCloseToNextZone(currentAbsoluteStep)) {
    --roundCounter;
    Serial.printf("Round counter is decreased: %d\n", roundCounter);
  }
}

void EncoderManager::runLoop() {
  for (;;)
  {
    currentAbsoluteStep = this->ams5600.getRawAngle();
    checkSegment();
    encoderData.absoluteStep = currentAbsoluteStep;
    encoderData.roundCounter = roundCounter;
    xQueueSend(qEncoderTask, &encoderData, portMAX_DELAY);
    previousAbsoluteStep = currentAbsoluteStep;
    vTaskDelay(1000);
  }
}
