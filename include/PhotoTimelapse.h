#pragma once

#include "Init.h"
#include "global.h"

class PhotoTimelapse {
  public:
    PhotoTimelapse(StaticJsonDocument<256> initParamsJsonDoc);
    void init();
    void run();
  private:
    xTimerHandle captureIntervalDelayTimer;

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

    bool isCaptureIntervalDelayFinished;

    enum class State {
      GOTO_NEXT,
      MOVING_UNTIL_TARGET,
      TARGET_REACHED,
      WAIT_FOR_NEXT
    } currentState;

    StaticJsonDocument<256> txJsonDoc, rxJsonDoc;

    //void moveToStartPosition();
    void gotoNextTargetPosition();
    bool checkIsTargetReached();
    void capturePhoto();
    void startCaptureIntervalDelayTimer();
    bool checkIsCaptureIntervalDelayFinished();
    void changeStateTo(State nextState);

    void onCaptureIntervalDelayTimeout();
    
    void IMotor_move(int step);
    void IMotor_moveTo(int targetPosition);
    bool IMotor_checkMotorSyncNotification();
    void ICamera_triggerCapture();
};
