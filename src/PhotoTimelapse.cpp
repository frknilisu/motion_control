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

    // create timer and its callback
    auto onTimer = [](xTimerHandle pxTimer){ 
        PhotoTimelapse* pt = static_cast<PhotoTimelapse*>(pvTimerGetTimerID(pxTimer)); // Retrieve the pointer to class
        pt->onIntervalDelayTimeout();
    };

    this->intervalDelayTimer = xTimerCreate(
        "CaptureIntervalDelayTimer",
        pdMS_TO_TICKS(this->capture_interval * 1000),
        pdFALSE,
        static_cast<void*>(this),
        onTimer);
}

void PhotoTimelapse::run() {
    if(iter_count >= this->number_of_photo)
        return;

    switch (status)
    {
        case "GOTO_NEXT":
            gotoNextTargetPosition();
            break;
        case "MOVING_UNTIL_TARGET":
            checkIsTargetReached();
            break;
        case "TARGET_REACHED":
            capturePhoto();
            startIntervalDelayTimer();
            break;
        case "WAIT_FOR_NEXT":
            checkIsIntervalDelayFinished();
            break;
        default:
            break;
    }
}

void PhotoTimelapse::moveToStartPosition() {
    Serial.println(">>>>>>>> PhotoTimelapse::moveToStartPosition() >>>>>>>>");

    if (this->direction == "a2b")
        IMotor_moveTo(this->pa);
    else
        IMotor_moveTo(this->pb);
    
    this->waitMotorSync();
}

void PhotoTimelapse::gotoNextTargetPosition() {
    IMotor_move(this->step_interval);
}

bool PhotoTimelapse::checkIsTargetReached() {
    return IMotor_checkMotorSyncNotification();
}

void PhotoTimelapse::capturePhoto() {
    ICamera_triggerCapture();
}

void PhotoTimelapse::startIntervalDelayTimer() {
    this->isIntervalDelayFinished = false;
    if(xTimerIsTimerActive(this->intervalDelayTimer) == pdFALSE) {
        xTimerStart(this->intervalDelayTimer, 0);
    }
}

bool PhotoTimelapse::checkIsIntervalDelayFinished() {
    return this->isIntervalDelayFinished;
}

void PhotoTimelapse::onIntervalDelayTimeout() {
    this->isIntervalDelayFinished = true;
}

void PhotoTimelapse::IMotor_move(long step) {
    Serial.println(">>>>>>>> IMotor_move() >>>>>>>>");

    StaticJsonDocument<CAPACITY> motorCommand;
    motorCommand["cmd"] = "MOTOR_MOVE_CMD";
    motorCommand["relative"] = step;

    xQueueSend(qMotorTask, &motorCommand, 0);
}

void PhotoTimelapse::IMotor_moveTo(long targetPosition) {
    Serial.println(">>>>>>>> IMotor_moveTo() >>>>>>>>");

    StaticJsonDocument<CAPACITY> motorCommand;
    motorCommand["cmd"] = "MOTOR_MOVE_TO_CMD";
    motorCommand["absolute"] = targetPosition;

    xQueueSend(qMotorTask, &motorCommand, 0);
}

bool IMotor_checkMotorSyncNotification() {
    Serial.println(">>>>>>>> IMotor_checkMotorSyncNotification() >>>>>>>>");

    // wait until motor finish movement
    xReturn = xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
    if(xReturn == pdPASS) {
      Serial.println("--- Notif received by ActionManager::PhotoTimelapse ---");
      return true;
    } else return false;
}

void ICamera_triggerCapture() {
    Serial.println(">>>>>>>> ICamera_triggerCapture() >>>>>>>>");
    xTaskNotifyGive(captureTaskHandle);

    //xTaskNotifyGive(actionTaskHandle);
}