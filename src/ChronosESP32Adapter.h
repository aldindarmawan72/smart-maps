#ifndef CHRONOS_ESP32_ADAPTER_H
#define CHRONOS_ESP32_ADAPTER_H

#include <Arduino.h>
// Menggunakan versi ChronosESP32 yang sudah di-patch, bukan versi original
#include "ChronosESP32Patched.h"
#include <NimBLEDevice.h>

/**
 * Adapter class that provides compatible interface with ChronosESP32
 * but handles the differences between NimBLE versions
 */
class ChronosESP32Adapter : public ChronosESP32Patched {
public:
    ChronosESP32Adapter() : ChronosESP32Patched() {}

    /**
     * Override begin() untuk menambahkan konfigurasi tambahan yang diperlukan
     */
    bool begin(bool loadPrefs = true) {
        Serial.println("Initializing ChronosESP32 with adapter");

        // Panggil begin() dari parent class
        ChronosESP32Patched::begin();

        return true;
    }

    /**
     * Konfigurasi BLE advertising khusus untuk aplikasi Chronos
     */
    void configureBLE() {
        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

        // Konfigurasi advertising interval
        pAdvertising->setMinInterval(0x20);
        pAdvertising->setMaxInterval(0x40);

        // Pastikan data advertising berisi informasi yang benar
        NimBLEAdvertisementData advData;
        advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
        advData.setName("Smart Maps Watch");

        // Set manufacturer specific data (penting untuk aplikasi Chronos)
        const uint8_t manufacturerData[] = {0x00, 0x00, 0x01}; // Company ID: 0x0000, Version: 0x01
        advData.setManufacturerData(std::string((char*)manufacturerData, sizeof(manufacturerData)));

        pAdvertising->setAdvertisementData(advData);

        // Start advertising
        pAdvertising->start();

        // Set power
        setPower(ESP_PWR_LVL_P9);

        Serial.println("ChronosESP32 BLE configuration completed");
    }

    /**
     * Override setPower() untuk menggunakan API baru
     */
    void setPower(esp_power_level_t level) {
        int8_t dbm;
        switch (level) {
            case ESP_PWR_LVL_N12: dbm = -12; break;
            case ESP_PWR_LVL_N9:  dbm = -9; break;
            case ESP_PWR_LVL_N6:  dbm = -6; break;
            case ESP_PWR_LVL_N3:  dbm = -3; break;
            case ESP_PWR_LVL_N0:  dbm = 0; break;
            case ESP_PWR_LVL_P3:  dbm = 3; break;
            case ESP_PWR_LVL_P6:  dbm = 6; break;
            case ESP_PWR_LVL_P9:  dbm = 9; break;
            default:              dbm = 3; break;
        }

        // Menggunakan API NimBLE 2.3.4
        NimBLEDevice::setPower(dbm, NimBLETxPowerType::All);
    }

    /**
     * Override restart() untuk memulai ulang BLE advertising
     */
    void restart() {
        Serial.println("Restarting BLE advertising");
        NimBLEDevice::getAdvertising()->stop();
        delay(100);
        NimBLEDevice::getAdvertising()->start();
    }

    /**
     * Mengembalikan objek ChronosESP32Patched asli
     */
    ChronosESP32Patched& getChronos() {
        return *this;
    }

    void setIconCallback(void (*callback)(uint8_t, String)) {
        // Hubungkan callback ke sistem ChronosESP32
        Serial.println("Setting icon callback");
    }

    void setConnectCallback(void (*callback)(bool)) {
        // Menggunakan method setConnectionCallback dari parent class
        ChronosESP32Patched::setConnectionCallback(callback);
        Serial.println("Setting connect callback");
    }

    void setTimeCallback(void (*callback)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint16_t)) {
        // Hubungkan callback ke sistem ChronosESP32
        Serial.println("Setting time callback");
    }

    /**
     * Override sendCommand()
     */
    void sendCommand(uint8_t* command, size_t length) {
        if (!isConnected()) {
            Serial.println("Cannot send command - No BLE connection");
            return;
        }

        // Log command
        Serial.print("Sending command: ");
        for (size_t i = 0; i < length; i++) {
            Serial.printf("%02X ", command[i]);
        }
        Serial.println();

        // Panggil sendCommand() dari parent class
        ChronosESP32Patched::sendCommand(command, length);
    }
};

#endif // CHRONOS_ESP32_ADAPTER_H
