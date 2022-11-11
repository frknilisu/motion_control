#pragma once

#include "Init.h"
#include "global.h"

class PhotoTimelapse {
  public:
    PhotoTimelapse(StaticJsonDocument<256> initParamsJsonDoc);
    void init();
    void run();
  private:
    xTimerHandle intervalDelayTimer;

    int record_duration;
    int video_duration;
    int fps;
    int shutter_speed;
    int number_of_photo;
    int capture_interval;

    int pa;
    int pb;
    std::string direction;
    int step_diff;
    int step_interval;

    bool isIntervalDelayFinished;

    StaticJsonDocument<256> txJsonDoc, rxJsonDoc;

    void moveToStartPosition();
    void gotoNextTargetPosition();
    bool checkIsTargetReached();
    void capturePhoto();
    void startIntervalDelayTimer();
    bool checkIsIntervalDelayFinished();

    void onIntervalDelayTimeout();
    
    void IMotor_move(long step);
    void IMotor_moveTo(long targetPosition);
    bool IMotor_checkMotorSyncNotification();
    void ICamera_triggerCapture();
};
