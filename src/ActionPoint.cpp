#include "ActionPoint.h"

ActionPoint::ActionPoint() {}

ActionPoint::ActionPoint(int segment, int remainder) 
  : m_segment(segment),
    m_remainder(remainder) {
  
}

int ActionPoint::getSegment() const {
  return this->m_segment;
}

int ActionPoint::getRemainder() const {
  return this->m_remainder;
}


void ActionPoint::setSegment(int segment) {
  this->m_segment = segment;
}

void ActionPoint::setRemainder(int remainder) {
  this->m_remainder = remainder;
}

void ActionPoint::print() {
  Serial.printf("print point cs/sc: %d / %d\n", this->m_remainder, this->m_segment);
}
