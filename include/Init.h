#ifndef INIT_H
#define INIT_H

#include <Arduino.h>

#define joyX 2
#define joyY 4
#define joySW 14

typedef struct EncoderData
{
  uint32_t absoluteStep;
  uint32_t roundCounter;
} EncoderData_t;

typedef struct MotorPositionData
{
  uint32_t currentStepPosition;
} MotorPositionData_t;

typedef enum Commands
{
  START_PROGRAMMING_CMD,
  FINISH_PROGRAMMING_CMD,
  SET_A_CMD,
  SET_B_CMD,
  MOTOR_RUN_CMD,
  MOTOR_STOP_CMD,
  MOTOR_START_ACTION_CMD,
  ACTION_FINISH_MSG
} Commands_t;

typedef struct QueueJsonMsg
{
  char* data;
} QueueJsonMsg_t;

enum class FPS_Opt
{
  _12,
  _24,
  _25,
  _30,
  _48,
  _50,
  _60,
  _96,
  _100,
  _120
};

enum class Shutter_Speed_Opt
{
  _60,
  _30,
  _15,
  _8,
  _4,
  _2,
  _1,
  _1_div_2,
  _1_div_4,
  _1_div_8,
  _1_div_15,
  _1_div_30,
  _1_div_60,
  _1_div_80,
  _1_div_125,
  _1_div_250,
  _1_div_500,
  _1_div_1000,
  _1_div_2000,
  _1_div_4000,
  _1_div_8000
};

typedef struct Duration
{
  int hour;
  int min;
  int sec;
} Duration_t;

typedef struct VideoTimelapses
{
  Duration_t record_duration; // hour + min + sec
  uint16_t video_duration; // sec
  FPS_Opt fps;        // FPS enum
  Shutter_Speed_Opt shutter_speed; // shutter speed enum
  uint16_t n_frame;
} VideoTimelapses_t;

enum class ActionType_t
{
  VideoTimelapsee
};

template <typename T>
struct ActionCmd
{
  ActionType_t actionType;
  T actionParams;
};


#endif
