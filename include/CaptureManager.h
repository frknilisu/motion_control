#ifndef CAPTURE_MANAGER_H
#define CAPTURE_MANAGER_H

#include "Init.h"
#include "global.h"

class CaptureManager {
  public:
    CaptureManager();
    void init();
    void runLoop();
  private:
    void waitTrigger();
};

#endif