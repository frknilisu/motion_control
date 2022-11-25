#include "BleManager.h"

#include <BLEDevice.h>
#include <BLE2902.h>

int val2;
char str2[80];

bool deviceConnected = false;
bool oldDeviceConnected = false;
std::string lastReceivedMsg;
StaticJsonDocument<256> receivedDoc;

/*------------------------------------------------------*/
/*---------------------- Callbacks ---------------------*/
/*------------------------------------------------------*/

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("***onConnect***");
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    Serial.println("***onDisconnect***");
    deviceConnected = false;
  }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (!rxValue.empty()) {
      lastReceivedMsg.append(rxValue);
    }
  }
};

BleManager::BleManager() :
  stateAdvertising([this]() { advertising_enter(); },
            [this]() { advertising_on(); },
            [this]() { advertising_exit(); }),
            
  stateConnected([this]() { connected_enter(); },
          [this]() { connected_on(); },
          [this]() { connected_exit(); }),
  
  fsm(&stateAdvertising),

  currentState(StateEnum::ADVERTISING) {
  Serial.println(">>>>>>>> BleManager() >>>>>>>>");
}

void BleManager::init() {
  Serial.println(">>>>>>>> BleManager::init() >>>>>>>>");
  
  BLEDevice::init("MyESP32 BLE");
  
  // Create the BLE Server
  this->pServer = BLEDevice::createServer();
  this->pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE UART Service
  this->pService = this->pServer->createService(SERVICE_UUID_UART);

  // Create a BLE Characteristic
  this->pTxCharacteristic = this->pService->createCharacteristic(
                              CHARACTERISTIC_UUID_TX,
                              BLECharacteristic::PROPERTY_NOTIFY
                            );

  this->pTxCharacteristic->addDescriptor(new BLE2902());

  this->pRxCharacteristic = this->pService->createCharacteristic(
                              CHARACTERISTIC_UUID_RX,
                              BLECharacteristic::PROPERTY_WRITE
                            );
  
  this->pRxCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  fsm.add_transition(&stateAdvertising, &stateConnected, CONNECT_EVENT, nullptr );
  fsm.add_transition(&stateConnected, &stateAdvertising, DISCONNECT_EVENT, nullptr );
  
}

void BleManager::runLoop() {
  for (;;) {
    fsm.run_machine();
    vTaskDelay(1000);
  }
}

/*--------------------------------------------------------------*/
/*---------------------- Utility Functions ---------------------*/
/*--------------------------------------------------------------*/

void BleManager::handleMsg(std::string receivedMsg) {
  deserializeJson(receivedDoc, receivedMsg.c_str());
  lastReceivedMsg.clear();
  const char* cmd = receivedDoc["cmd"];
  switch(hashit(cmd)) {
    case BLEMsgsEnum::msg_StartProgramming:
      Serial.println("-- Received Msg: startProgramming --");

      txJsonDoc["target"] = "MissionController";
      txJsonDoc["cmd"] = "START_PROGRAMMING_CMD";

      xQueueSend(qMissionTaskCmd, &txJsonDoc, eSetValueWithOverwrite);

      break;
    case BLEMsgsEnum::msg_FinishProgramming:
      Serial.println("-- Received Msg: finishProgramming --");

      txJsonDoc["target"] = "MissionController";
      txJsonDoc["cmd"] = "FINISH_PROGRAMMING_CMD";

      xQueueSend(qMissionTaskCmd, &txJsonDoc, eSetValueWithOverwrite);
      
      break;
    case BLEMsgsEnum::msg_SetA:
      Serial.println("-- Received Msg: setA --");
      
      txJsonDoc["target"] = "MissionController";
      txJsonDoc["cmd"] = "SET_A_CMD";

      xQueueSend(qMissionTaskCmd, &txJsonDoc, eSetValueWithOverwrite);

      break;
    case BLEMsgsEnum::msg_SetB:
      Serial.println("-- Received Msg: setB --");

      txJsonDoc["target"] = "MissionController";
      txJsonDoc["cmd"] = "SET_B_CMD";

      xQueueSend(qMissionTaskCmd, &txJsonDoc, eSetValueWithOverwrite);

      break;
    case BLEMsgsEnum::msg_SetActionData:
      Serial.println("-- Received Msg: setActionData --");

      //txJsonDoc["target"] = "MissionController";
      receivedDoc["cmd"] = "SET_ACTION_DATA_CMD";
      //txJsonDoc["data"] = receivedDoc["data"];

      xQueueSend(qMissionTaskCmd, &receivedDoc, eSetValueWithOverwrite);

      break;
    case BLEMsgsEnum::msg_MotorRun:
      Serial.println("-- Received Msg: motorRun --");

      txJsonDoc["target"] = "MotorManager";
      txJsonDoc["cmd"] = "MOTOR_RUN_CMD";

      xQueueSend(qMotorTask, &txJsonDoc, eSetValueWithOverwrite);

      break;
    case BLEMsgsEnum::msg_MotorStop:
      Serial.println("-- Received Msg: motorStop --");

      txJsonDoc["target"] = "MotorManager";
      txJsonDoc["cmd"] = "MOTOR_STOP_CMD";

      xQueueSend(qMotorTask, &txJsonDoc, eSetValueWithOverwrite);
      
      break;
    case BLEMsgsEnum::msg_ManualDrive:
      Serial.println("-- Received Msg: manualDrive --");

      txJsonDoc["target"] = "MotorManager";
      txJsonDoc["cmd"] = "MOTOR_SET_SPEED_CMD";
      txJsonDoc["speed"] = receivedDoc["speed"];

      xQueueSend(qMotorTask, &txJsonDoc, eSetValueWithOverwrite);
      
      break;
  }
}

BleManager::BLEMsgsEnum BleManager::hashit(std::string const& inString) {
  if (inString == "startProgramming") return BLEMsgsEnum::msg_StartProgramming;
  if (inString == "finishProgramming") return BLEMsgsEnum::msg_FinishProgramming;
  if (inString == "setA") return BLEMsgsEnum::msg_SetA;
  if (inString == "setB") return BLEMsgsEnum::msg_SetB;
  if (inString == "setActionData") return BLEMsgsEnum::msg_SetActionData;
  if (inString == "motorRun") return BLEMsgsEnum::msg_MotorRun;
  if (inString == "motorStop") return BLEMsgsEnum::msg_MotorStop;
  if (inString == "manualDrive") return BLEMsgsEnum::msg_ManualDrive;
}

bool BleManager::isDeviceConnected() {
  return this->currentState == StateEnum::CONNECTED;
}

void BleManager::startAdvertising() {
  Serial.println("--- BLE -> START_ADVERTISING ---");

  // Start the service
  this->pService->start();

  // Start advertising
  this->pServer->getAdvertising()->start();
}

/*--------------------------------------------------------------*/
/*---------------------- State Functions -----------------------*/
/*--------------------------------------------------------------*/

void BleManager::advertising_enter() {
  Serial.println("--- Enter: BleManager -> ADVERTISING ---");
  this->startAdvertising();
}

void BleManager::advertising_on() {
  //Serial.println("--- Update: BleManager -> ADVERTISING ---");

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    fsm.trigger(CONNECT_EVENT);
  }
}

void BleManager::advertising_exit() {
  Serial.println("--- Exit: BleManager -> ADVERTISING ---");
}

void BleManager::connected_enter() {
  Serial.println("--- Enter: BleManager -> CONNECTED ---");
}

void BleManager::connected_on() {
  //Serial.println("--- Update: BleManager -> CONNECTED ---");

  if(!lastReceivedMsg.empty()) {
    this->handleMsg(lastReceivedMsg);
  }
  if(!deviceConnected) {
    fsm.trigger(DISCONNECT_EVENT);
  }
}

void BleManager::connected_exit() {
  Serial.println("--- Exit: BleManager -> CONNECTED ---");
  if (!deviceConnected && oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    vTaskDelay(100);
  }
}
