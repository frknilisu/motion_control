#ifndef ACTION_POINT_H
#define ACTION_POINT_H

#include <Arduino.h>

class ActionPoint {
  public:
    ActionPoint();
    ActionPoint(int segment, int remainder);
    int getSegment() const;
    int getRemainder() const;
    void setSegment(int segment);
    void setRemainder(int remainder);
    void print();
    
  private:
    int m_segment;      // counter
    int m_remainder;    // raw encoder
};

#endif
