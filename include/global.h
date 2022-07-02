#ifndef GLOBAL_H
#define GLOBAL_H

extern TaskHandle_t missionControlTaskHandle;
extern TaskHandle_t bleTaskHandle;
extern TaskHandle_t motorTaskHandle;
extern TaskHandle_t encoderTaskHandle;

extern QueueHandle_t qEncoderTask;
extern QueueHandle_t qMotorTask;
extern QueueHandle_t qMissionControlTask;

extern BaseType_t xReturn;

#endif