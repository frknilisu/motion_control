#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#include <Arduino.h>

#include "Pair.h"
#include "ActionPoint.h"

#define ENCODER_RESOLUTION  12
#define MOTOR_RESOLUTION    1/8

namespace Calculations
{ 
  Pair<int, int> pointDiffOnSameSegment(const ActionPoint& p1, const ActionPoint& p2) {
    int rawRemainderDiff = p2.getRemainder() - p1.getRemainder();
    int dir = rawRemainderDiff >= 0 ? 1 : -1;
    return Pair<int, int>(dir, rawRemainderDiff);
  }
  
  int pair2StepDiff(Pair<int, int> tempPointDiff) {
    return int(tempPointDiff.first()) * tempPointDiff.second();
  }
  
  Pair<int, int> step2PairDiff(int encoderStepDiff) {
    return Pair<int, int>(encoderStepDiff >= 0 ? 1 : -1, abs(encoderStepDiff));
  }
  
  Pair<int, int> calculatePointDifference(const ActionPoint& p1, const ActionPoint& p2) {
    if(p1.getSegment() == p2.getSegment())
      return pointDiffOnSameSegment(p1, p2);
    
    int rawSegmentDiff = p2.getSegment() - p1.getSegment();
    int encoderStepDiff = 2^ENCODER_RESOLUTION * rawSegmentDiff;
    Pair<int, int> tempPointDiff = pointDiffOnSameSegment(ActionPoint(p2.getSegment(), p1.getRemainder()), p2);
    int tempStepDiff = pair2StepDiff(tempPointDiff); 
    encoderStepDiff += tempStepDiff;
    return step2PairDiff(encoderStepDiff);
  }
  
  int convertEncoderStepToMotorStep(int encoderStep) {
    return round(encoderStep * (2^ENCODER_RESOLUTION / (200 / MOTOR_RESOLUTION)));
  }
}

#endif
