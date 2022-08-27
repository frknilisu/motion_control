PhotoTimelapse::PhotoTimelapse(StaticJsonDocument<200> initParamsJsonDoc) {
    record_duration = initParamsJsonDoc["record_duration"]; // 60 * 60
    video_duration = initParamsJsonDoc["video_duration"]; // 30
    fps = initParamsJsonDoc["fps"]; // 24
    shutter_speed = initParamsJsonDoc["shutter_speed"]; // "1_500"
    number_of_photo = video_duration * fps; // 720
    capture_interval = record_duration / number_of_photo; // 5
    
    pa = initParamsJsonDoc["pa"];
    pb = initParamsJsonDoc["pb"];
    direction = initParamsJsonDoc["direction"];
    step_diff = direction == "a2b" ? pb - pa : pa - pb;
    step_interval = step_diff / number_of_photo;
}

void PhotoTimelapse::init() {
    timer = xCreateTimer();
}

void waitMoveToHome() {
    if (direction == "a2b")
        moveTo(pa);
    else
        moveTo(pb);
    waitMotorSync();
}

void waitMoveToNextPosition() {
    move(step_interval);
    waitMotorSync();
}

void move(int step) {
    // motor move relative step command
    // send queue msg
}

void moveTo(int position) {
    // motor move absoulate position command
    // send queue msg
}

void waitMotorSync() {
    // wait until motor finish movement
    // receive queue msg
}

void prerun() {
    waitMoveToHome();
    triggerCapture();
    timer.start();
}

void onTimeout() {
    triggerCapture();
    unclock();
}

void PhotoTimelapse::run() {
    for(int i = 0; i < number_of_photo; i++){
        waitMoveToNextPosition();
        lock();
    }
}