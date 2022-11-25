#include "PhotoTimelapse.h"

const size_t CAPACITY = 256;

static int iter_count = 0;
StaticJsonDocument<CAPACITY> data, motor_data, capture_data;

PhotoTimelapse::PhotoTimelapse(StaticJsonDocument<256> initParamsJson) {
    Serial.println(">>>>>>>> PhotoTimelapse(initParams) >>>>>>>>");
    serializeJson(initParamsJson, Serial);
    
    data = initParamsJson["data"];
    motor_data = data["motor"];
    capture_data = data["capture"];

    this->record_duration = capture_data["record_duration"]; // 60 * 60
    this->video_duration = capture_data["video_duration"]; // 30
    this->fps = capture_data["fps"]; // 24
    this->shutter_speed = capture_data["shutter_speed"]; // "1_500"
    this->number_of_photo = video_duration * fps; // 720
    this->capture_interval = record_duration / number_of_photo; // 5
    
    this->pa = motor_data["pa"];
    this->pb = motor_data["pb"];
    Serial.println(this->pa);
    Serial.println(this->pb);
    const char* s = motor_data["direction"]; 
    this->direction = s;
    this->step_diff = this->direction == "a2b" ? this->pb - this->pa : this->pa - this->pb;
    this->step_interval = this->step_diff / this->number_of_photo;

    Serial.println(this->record_duration);
    Serial.println(this->video_duration);
    Serial.println(this->fps);
    Serial.println(this->number_of_photo);

    Serial.println(this->step_diff);
    Serial.println(this->step_interval);

    this->init();
}

void PhotoTimelapse::init() {
    Serial.println(">>>>>>>> PhotoTimelapse::init() >>>>>>>>");

    this->capture_interval = 15;

    // create timer and its callback
    auto onTimer = [](xTimerHandle pxTimer){ 
        PhotoTimelapse* pt = static_cast<PhotoTimelapse*>(pvTimerGetTimerID(pxTimer)); // Retrieve the pointer to class
        pt->onCaptureIntervalDelayTimeout();
    };

    this->captureIntervalDelayTimer = xTimerCreate(
        "CaptureIntervalDelayTimer",
        pdMS_TO_TICKS(this->capture_interval * 1000),
        pdFALSE,
        static_cast<void*>(this),
        onTimer);

    this->currentState = State::GOTO_NEXT;

    //this->record_duration = 60 * 60;
    //this->video_duration = 30;
    //this->fps = 24;
    //this->shutter_speed = "1_500";
    //this->number_of_photo = 720;
    

    this->pa = 0;
    this->pb = 100*40*8;
    this->direction = "a2b";
    this->step_diff = this->direction == "a2b" ? this->pb - this->pa : this->pa - this->pb;
    this->step_interval = 100;

}

void PhotoTimelapse::run() {
    if(iter_count >= this->number_of_photo)
        return;

    switch (this->currentState)
    {
        case State::GOTO_NEXT:
            Serial.println(">>>>>>>> State::GOTO_NEXT >>>>>>>>");
            startCaptureIntervalDelayTimer();
            gotoNextTargetPosition();
            this->changeStateTo(State::MOVING_UNTIL_TARGET);
            break;
        case State::MOVING_UNTIL_TARGET:
            Serial.println(">>>>>>>> State::MOVING_UNTIL_TARGET >>>>>>>>");
            if(checkIsTargetReached()) {
                this->changeStateTo(State::TARGET_REACHED);
            }
            break;
        case State::TARGET_REACHED:
            Serial.println(">>>>>>>> State::TARGET_REACHED >>>>>>>>");
            capturePhoto();
            this->changeStateTo(State::WAIT_FOR_NEXT);
            break;
        case State::WAIT_FOR_NEXT:
            Serial.println(">>>>>>>> State::WAIT_FOR_NEXT >>>>>>>>");
            if(checkIsCaptureIntervalDelayFinished()) {
                this->changeStateTo(State::GOTO_NEXT);
            }
            break;
        default:
            break;
    }
}

/*
void PhotoTimelapse::moveToStartPosition() {
    Serial.println(">>>>>>>> PhotoTimelapse::moveToStartPosition() >>>>>>>>");

    if (this->direction == "a2b")
        IMotor_moveTo(this->pa);
    else
        IMotor_moveTo(this->pb);
    
    this->waitMotorSync();
}
*/

void PhotoTimelapse::gotoNextTargetPosition() {
    Serial.println(">>>>>>>> PhotoTimelapse::gotoNextTargetPosition() >>>>>>>>");
    IMotor_move(this->step_interval);
}

bool PhotoTimelapse::checkIsTargetReached() {
    Serial.println(">>>>>>>> PhotoTimelapse::checkIsTargetReached() >>>>>>>>");
    return IMotor_checkMotorSyncNotification();
}

void PhotoTimelapse::capturePhoto() {
    Serial.println(">>>>>>>> PhotoTimelapse::capturePhoto() >>>>>>>>");
    ICamera_triggerCapture();
}

void PhotoTimelapse::startCaptureIntervalDelayTimer() {
    Serial.println(">>>>>>>> PhotoTimelapse::startCaptureIntervalDelayTimer() >>>>>>>>");
    this->isCaptureIntervalDelayFinished = false;
    if(xTimerIsTimerActive(this->captureIntervalDelayTimer) == pdFALSE) {
        xTimerStart(this->captureIntervalDelayTimer, 0);
    }
}

bool PhotoTimelapse::checkIsCaptureIntervalDelayFinished() {
    Serial.println(">>>>>>>> PhotoTimelapse::checkIsCaptureIntervalDelayFinished() >>>>>>>>");
    return this->isCaptureIntervalDelayFinished;
}

void PhotoTimelapse::changeStateTo(State nextState) {
    Serial.println(">>>>>>>> PhotoTimelapse::changeStateTo() >>>>>>>>");
    this->currentState = nextState;
}

void PhotoTimelapse::onCaptureIntervalDelayTimeout() {
    Serial.println(">>>>>>>> PhotoTimelapse::onCaptureIntervalDelayTimeout() >>>>>>>>");
    this->isCaptureIntervalDelayFinished = true;
}

void PhotoTimelapse::IMotor_move(int step) {
    Serial.println(">>>>>>>> IMotor_move() >>>>>>>>");

    //StaticJsonDocument<CAPACITY> motorCommand;
    txJsonDoc.clear();
    txJsonDoc["cmd"] = "MOTOR_MOVE_CMD";
    txJsonDoc["relative"] = step;

    xQueueSend(qMotorTask, &txJsonDoc, 0);
}

void PhotoTimelapse::IMotor_moveTo(int targetPosition) {
    Serial.println(">>>>>>>> IMotor_moveTo() >>>>>>>>");

    //StaticJsonDocument<CAPACITY> motorCommand;
    txJsonDoc.clear();
    txJsonDoc["cmd"] = "MOTOR_MOVE_TO_CMD";
    txJsonDoc["absolute"] = targetPosition;

    xQueueSend(qMotorTask, &txJsonDoc, 0);
}

bool PhotoTimelapse::IMotor_checkMotorSyncNotification() {
    Serial.println(">>>>>>>> IMotor_checkMotorSyncNotification() >>>>>>>>");

    // wait until motor finish movement
    uint32_t receivedValue;
    xReturn = xTaskNotifyWait(0, 0, &receivedValue, portMAX_DELAY);
    if(xReturn == pdPASS && receivedValue == 32) {
      Serial.println("--- Notif received by ActionManager::PhotoTimelapse ---");
      return true;
    } else return false;
}

void PhotoTimelapse::ICamera_triggerCapture() {
    Serial.println(">>>>>>>> ICamera_triggerCapture() >>>>>>>>");
    xTaskNotifyGive(captureTaskHandle);

    //xTaskNotifyGive(actionTaskHandle);
}