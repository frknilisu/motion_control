#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include "Init.h"
#include "global.h"
#include <Arduino.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define SERVICE_UUID_UART      "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define CONNECT_EVENT 1
#define DISCONNECT_EVENT 2

class BleManager {
  public:
    BleManager();
    void init();
    void runLoop();
  private:
    enum class StateEnum {
      UNINTIALIZED,
      ADVERTISING,
      CONNECTED
    } currentState;

    enum class BLEMsgsEnum {
      msg_StartProgramming,
      msg_FinishProgramming,
      msg_SetA,
      msg_SetB,
      msg_MotorRun,
      msg_MotorStop
    };

    bool isNewMessageExist = false;
    StaticJsonDocument<200> txJsonDoc, rxJsonDoc;
    
    BLEServer* pServer = NULL;
    BLEService* pService = NULL;
    BLECharacteristic* pTxCharacteristic = NULL;
    BLECharacteristic* pRxCharacteristic = NULL;
    
    void handleMsg(std::string receivedMsg);
    BLEMsgsEnum hashit(std::string const& inString);
    bool isDeviceConnected();
    void startAdvertising();
    void notifyEncoder();

    void advertising_enter();
    void advertising_on();
    void advertising_exit();

    void connected_enter();
    void connected_on();
    void connected_exit();

    FunctionState stateAdvertising;
    FunctionState stateConnected;

    FunctionFsm fsm;
};


#endif
