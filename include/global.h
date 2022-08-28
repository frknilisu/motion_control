#ifndef GLOBAL_H
#define GLOBAL_H

extern QueueHandle_t qMotorTask;
extern QueueHandle_t qMissionTask;
extern QueueHandle_t qCaptureTask;
extern QueueHandle_t qBleTask;
extern QueueHandle_t qActionTask;

extern TaskHandle_t captureTaskHandle;
extern TaskHandle_t actionTaskHandle;

extern BaseType_t xReturn;

#endif