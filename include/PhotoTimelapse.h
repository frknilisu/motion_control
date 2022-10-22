#pragma once

#include "Init.h"
#include "global.h"

class PhotoTimelapse {
  public:
    PhotoTimelapse(StaticJsonDocument<256> initParamsJsonDoc);
    void init();
    void run();
  private:
    xTimerHandle timer;

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

    StaticJsonDocument<256> txJsonDoc, rxJsonDoc;

    void prerun();
    void move(int step);
    void moveTo(int position);
    void waitMoveToHome();
    void waitMoveToNextPosition();
    void waitMotorSync();
    void triggerCapture();
    void onTimeout();
    
};
