#include <BLE.h>
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Debug.h>

namespace
{
    constexpr const char* DEVICE_NAME = "ESPWatch";
    constexpr const char* SERVICE_UUID = "ea007e77-c1a1-40e9-b802-fb8b4fa63cce";
    constexpr const char* READING_UUID = "0a0bc9b6-3333-4f1c-b18a-58b11d03fe0a";
    struct __attribute__((packed)) Reading {
        uint32_t uptimeMs;  // millis() when sent
        uint32_t steps;
        uint8_t heartBpm;
        bool isSleeping;
    };
    static_assert(sizeof(Reading) == 10, "update RECORD in scripts/stream.py if this changes");

    NimBLECharacteristic* readingChr = nullptr;
    Reading reading = {};

    void setReadingValue() {
        readingChr->setValue(reinterpret_cast<const uint8_t*>(&reading), sizeof(reading));
    }
}

namespace BLE {
    void setup() {
        NimBLEDevice::init(DEVICE_NAME);

        // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO);
        // NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM | BLE_SM_PAIR_AUTHREQ_SC);

        NimBLEServer* server = NimBLEDevice::createServer();
        server->advertiseOnDisconnect(true);
        NimBLEService* service = server->createService(SERVICE_UUID);

        readingChr = service->createCharacteristic(READING_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
        setReadingValue();

        NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
        advertising->setName(DEVICE_NAME);
        advertising->addServiceUUID(service->getUUID());

        advertising->enableScanResponse(false); // this saves battery life at the expense of less data sent
        advertising->start();

        DEBUG_PRINTLN("setup ble server");
    }

    void updateSteps(uint32_t steps) {
        reading.steps = steps;
    }

    void updateHeart(uint8_t bpm) {
        reading.heartBpm = bpm;
    }

    void send() {
        if (!readingChr) return;
        reading.uptimeMs = millis();
        setReadingValue();
        readingChr->notify();
    }
}
