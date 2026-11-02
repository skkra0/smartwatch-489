#include <NimBLEService.h>
#include <NimBLECharacteristic.h>
#include <NimBLEServer.h>
#include <NimBLEDevice.h>

#include <Debug.h>

namespace
{
    NimBLEServer* pServer;
}

namespace BLE {
    void setup() {
        NimBLEDevice::init("NimBLE");

        // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO);
        // NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM | BLE_SM_PAIR_AUTHREQ_SC);

        pServer = NimBLEDevice::createServer();
        NimBLEService* pService = pServer->createService("ABCD");

        NimBLECharacteristic *pCharacteristic = pService->createCharacteristic("1234",
                NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY // | NIMBLE_PROPERTY::READ_ENC
            );

        pCharacteristic->setValue("hi");

        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
        pAdvertising->setName("NimBLE");
        pAdvertising->addServiceUUID(pService->getUUID());

        pAdvertising->enableScanResponse(false); // this saves battery life at the expense of less data sent
        pAdvertising->start();

        DEBUG_PRINTLN("setup ble server");
    }

    // Notify connected peers
    void notify() {
        if (pServer->getConnectedCount()) {
            NimBLEService* pSvc = pServer->getServiceByUUID("ABCD");
            if (pSvc) {
                NimBLECharacteristic* pChr = pSvc->getCharacteristic("1234");
                if (pChr) {
                    pChr->notify();
                }
            }
        }
    }
}
