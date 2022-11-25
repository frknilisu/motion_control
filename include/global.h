#ifndef GLOBAL_H
#define GLOBAL_H

extern QueueHandle_t qMotorTask;
extern QueueHandle_t qMissionTaskData;
extern QueueHandle_t qMissionTaskCmd;
extern QueueHandle_t qCaptureTask;
extern QueueHandle_t qBleTask;
extern QueueHandle_t qActionTask;

extern TaskHandle_t missionControlTaskHandle;
extern TaskHandle_t bleTaskHandle;
extern TaskHandle_t motorTaskHandle;
extern TaskHandle_t actionTaskHandle;
extern TaskHandle_t captureTaskHandle;

extern BaseType_t xReturn;

#endif