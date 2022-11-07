#include "PhotoTimelapse.h"

static int iter_count = 0;
StaticJsonDocument<256> data, motor_data, capture_data;

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

    auto onTimer = [](xTimerHandle pxTimer){ 
        PhotoTimelapse* pt = static_cast<PhotoTimelapse*>(pvTimerGetTimerID(pxTimer)); // Retrieve the pointer to class
        pt->onTimeout();
    };

    this->timer = xTimerCreate(
        "timer1Sec", /* name */
        pdMS_TO_TICKS(30000), /* period/time */
        pdTRUE, /* auto reload */
        static_cast<void*>(this), /* timer ID */
        onTimer); /* callback */
}

void PhotoTimelapse::run() {
    if(iter_count >= this->number_of_photo)
        return;

    if(xTimerIsTimerActive(this->timer) == pdFALSE) {
        xTimerStart(this->timer, 0);
        this->triggerCapture();
    }
    
    this->waitMoveToNextPosition();
    xReturn = xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
    if(xReturn == pdPASS) {
      Serial.println("--- Notif received by ActionManager::PhotoTimelapse ---");
    }
}

void PhotoTimelapse::waitMoveToHome() {
    Serial.println(">>>>>>>> PhotoTimelapse::waitMoveToHome() >>>>>>>>");

    if (this->direction == "a2b")
        this->moveTo(this->pa);
    else
        this->moveTo(this->pb);
    
    this->waitMotorSync();
}

void PhotoTimelapse::waitMoveToNextPosition() {
    Serial.println(">>>>>>>> PhotoTimelapse::waitMoveToNextPosition() >>>>>>>>");

    this->move(this->step_interval);
    this->waitMotorSync();
    Serial.println(">>>>>>>> PhotoTimelapse -> motor sync is done >>>>>>>>");
}

void PhotoTimelapse::move(int step) {
    Serial.println(">>>>>>>> PhotoTimelapse::move() >>>>>>>>");

    // Send Motor Command
    txJsonDoc.clear();
    txJsonDoc["target"] = "MotorManager";
    txJsonDoc["cmd"] = "MOVE_RELATIVE";
    txJsonDoc["step"] = step;

    // send queue msg
    xQueueSend(qMotorTask, &txJsonDoc, 0);
}

void PhotoTimelapse::moveTo(int position) {
    Serial.println(">>>>>>>> PhotoTimelapse::moveTo() >>>>>>>>");

    // Send Motor Command
    txJsonDoc.clear();
    txJsonDoc["target"] = "MotorManager";
    txJsonDoc["cmd"] = "MOVE_ABSOULATE";
    txJsonDoc["position"] = position;

    // send queue msg
    xQueueSend(qMotorTask, &txJsonDoc, 0);
}

void PhotoTimelapse::waitMotorSync() {
    Serial.println(">>>>>>>> PhotoTimelapse::waitMotorSync() >>>>>>>>");

    // wait until motor finish movement
    xReturn = xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
    if(xReturn == pdPASS) {
      Serial.println("--- Notif received by ActionManager::PhotoTimelapse ---");
    }
}

void PhotoTimelapse::onTimeout() {
    this->triggerCapture();
    xTaskNotifyGive(actionTaskHandle);
}

void PhotoTimelapse::triggerCapture() {
    Serial.println(">>>>>>>> PhotoTimelapse::triggerCapture() >>>>>>>>");
    xTaskNotifyGive(captureTaskHandle);
}