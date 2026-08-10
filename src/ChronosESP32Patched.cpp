/*
   MIT License

  Copyright (c) 2023 Felix Biego
  
  Modified/patched version to avoid conflicts with the project's Config namespace

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  ______________  _____
  ___  __/___  /_ ___(_)_____ _______ _______
  __  /_  __  __ \__  / _  _ \__  __ `/_  __ \
  _  __/  _  /_/ /_  /  /  __/_  /_/ / / /_/ /
  /_/     /_.___/ /_/   \___/ _\__, /  \____/
							  /____/

*/

#include "ChronosESP32Patched.h"

// Basic implementation of required methods

BLECharacteristic *ChronosESP32Patched::pCharacteristicTX = nullptr;
BLECharacteristic *ChronosESP32Patched::pCharacteristicRX = nullptr;

ChronosESP32Patched::ChronosESP32Patched() : ESP32Time(0) {
    _inited = false;
    _subscribed = false;
    _batteryLevel = 100;
    _isCharging = false;
    _connected = false;
    _batteryChanged = false;
    _hour24 = true;
    _cameraReady = false;
    _notificationIndex = 0;
    _weatherSize = 0;
    _weatherCity = "";
    _weatherTime = "";
    _appCode = 0;
    _appVersion = "";
    _touch.state = false;
    _touch.x = 0;
    _touch.y = 0;
    _sosContact = 0;
    _contactSize = 0;
    _screenConf = CS_240x240_128_CTF;
    _infoTimer.time = 0;
    _infoTimer.active = false;
    _findTimer.time = 0;
    _findTimer.active = false;
    _incomingData.length = 0;
    _outgoingData.length = 0;
    _sendESP = false;
    _chunked = false;
}

ChronosESP32Patched::ChronosESP32Patched(String name, ChronosScreen screen) : ESP32Time(0) {
    _watchName = name;
    _screenConf = screen;
    _inited = false;
    _subscribed = false;
    _batteryLevel = 100;
    _isCharging = false;
    _connected = false;
    _batteryChanged = false;
    _hour24 = true;
    _cameraReady = false;
    _notificationIndex = 0;
    _weatherSize = 0;
    _weatherCity = "";
    _weatherTime = "";
    _appCode = 0;
    _appVersion = "";
    _touch.state = false;
    _touch.x = 0;
    _touch.y = 0;
    _sosContact = 0;
    _contactSize = 0;
    _infoTimer.time = 0;
    _infoTimer.active = false;
    _findTimer.time = 0;
    _findTimer.active = false;
    _incomingData.length = 0;
    _outgoingData.length = 0;
    _sendESP = false;
    _chunked = false;
}

void ChronosESP32Patched::begin() {
    if (_inited) {
        return;
    }
    
    // Initialize BLE device
    // Convert from String to std::string
    NimBLEDevice::init(std::string(_watchName.c_str()));
    
    // Create server
    NimBLEServer *pServer = NimBLEDevice::createServer();
    BLEDevice::setMTU(517);
    pServer->setCallbacks(this);
    
    // Create service
    NimBLEService *pService = pServer->createService(SERVICE_UUID);
    
    // Create characteristics
    pCharacteristicTX = pService->createCharacteristic(
                            CHARACTERISTIC_UUID_TX,
                            NIMBLE_PROPERTY::NOTIFY);
    
    pCharacteristicRX = pService->createCharacteristic(
                            CHARACTERISTIC_UUID_RX,
                            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    
    pCharacteristicTX->setCallbacks(this);
    pCharacteristicRX->setCallbacks(this);
    
    // Start service
    pService->start();
    
    // Initialize advertising
    NimBLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->enableScanResponse(true);
	pAdvertising->setPreferredParams(0x06, 0x12); // functions that help with iPhone connections issue
	pAdvertising->setName(_watchName.c_str());
	pAdvertising->start();

    
    _inited = true;
    _address = NimBLEDevice::getAddress().toString().c_str();
    
    Serial.println("ChronosESP32Patched started: " + _address);
}

bool ChronosESP32Patched::isRunning() {
    return _inited;
}

void ChronosESP32Patched::stop(bool clearAll) {
    if (!_inited) {
        return;
    }
    
    NimBLEDevice::deinit(clearAll);
    _inited = false;
    _subscribed = false;
    _connected = false;
}

void ChronosESP32Patched::setName(String name) {
    _watchName = name;
}

void ChronosESP32Patched::setScreen(ChronosScreen screen) {
    _screenConf = screen;
}

bool ChronosESP32Patched::isConnected() {
    return _connected;
}

void ChronosESP32Patched::setConnectionCallback(void (*callback)(bool)) {
    connectionChangeCallback = callback;
}

void ChronosESP32Patched::setNotificationCallback(void (*callback)(Notification)) {
    notificationReceivedCallback = callback;
}

void ChronosESP32Patched::setRingerCallback(void (*callback)(String, bool)) {
    ringerAlertCallback = callback;
}

void ChronosESP32Patched::setConfigurationCallback(void (*callback)(ConfigType, uint32_t, uint32_t)) {
    configurationReceivedCallback = callback;
}

void ChronosESP32Patched::setDataCallback(void (*callback)(uint8_t *, int)) {
    dataReceivedCallback = callback;
}

void ChronosESP32Patched::setRawDataCallback(void (*callback)(uint8_t *, int)) {
    rawDataReceivedCallback = callback;
}

void ChronosESP32Patched::setHealthRequestCallback(void (*callback)(HealthRequest, bool)) {
    healthRequestCallback = callback;
}

void ChronosESP32Patched::onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) {
    _connected = true;
    
    if (connectionChangeCallback != nullptr) {
        connectionChangeCallback(_connected);
    }
}

void ChronosESP32Patched::onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) {
	_connected = false;
	BLEDevice::startAdvertising();
	_touch.state = false; // release touch

	if (_navigation.active)
	{
		_navigation.active = false;
		if (configurationReceivedCallback != nullptr)
		{
			configurationReceivedCallback(CF_NAV_DATA, _navigation.active ? 1 : 0, 0);
		}
	}

	if (connectionChangeCallback != nullptr)
	{
		connectionChangeCallback(false);
	}
    Serial.println("BLE disconnected, restarting advertising");
}

void ChronosESP32Patched::onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) {
    if (pCharacteristic->getUUID().toString() == CHARACTERISTIC_UUID_RX) {
        std::string pData = pCharacteristic->getValue();
        int len = pData.length();
        
        if (len > 0) {
            if (rawDataReceivedCallback != nullptr) {
                rawDataReceivedCallback((uint8_t *)pData.data(), len);
            }

            if ((pData[0] == 0xAB || pData[0] == 0xEA) && (pData[3] == 0xFE || pData[3] == 0xFF)) {
                // Start of data, get length from packet
                _incomingData.length = pData[1] * 256 + pData[2] + 3;
                
                // Copy data into the buffer
                for (int i = 0; i < len; i++) {
                    _incomingData.data[i] = pData[i];
                }

                if (_incomingData.length <= len) {
                    // Packet has been fully reassembled
                    dataReceived();
                }
                else {
                    // Data is still being reassembled
                    Serial.println("Packet incomplete, waiting for more data...");
                }
            }
            else {
                // This is the next part of the packet
                int j = 20 + (pData[0] * 19); // Packet data position
                
                // Copy data into the buffer
                for (int i = 0; i < len; i++) {
                    _incomingData.data[j + i] = pData[i + 1];
                }

                if (_incomingData.length <= len + j - 1) {
                    // Packet has been fully reassembled
                    dataReceived();
                }
                else {
                    // Data is still being reassembled
                    Serial.println("Packet still incomplete, waiting for more data...");
                }
            }
        }
    }
}

void ChronosESP32Patched::onSubscribe(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue) {
    _subscribed = true;
}

bool ChronosESP32Patched::isSubscribed() {
    return _subscribed;
}

void ChronosESP32Patched::sendCommand(uint8_t *command, size_t length, bool force_chunked) {
    if (!_inited || !_subscribed) {
        return;
    }
    
    if (length > 0) {
        if (pCharacteristicTX != nullptr) {
            pCharacteristicTX->setValue(command, length);
            pCharacteristicTX->notify();
        }
    }
}

void ChronosESP32Patched::dataReceived() {
    // Check whether there is data
    if (_incomingData.length < 1) {
        return;
    }
    int len = _incomingData.length;
    
    // Log raw data received (for debugging) with special formatting
    Serial.print("Received data: ");
    for (int i = 0; i < len; i++) {
        Serial.printf("%02X ", _incomingData.data[i]);
    }
    Serial.println();
    
    // Send to callback if the user wants to handle raw data
    if (rawDataReceivedCallback != nullptr) {
        rawDataReceivedCallback(_incomingData.data, _incomingData.length);
    }
    
    // Send data to the general callback if registered
    if (dataReceivedCallback != nullptr) {
        dataReceivedCallback(_incomingData.data, _incomingData.length);
    }
    
    // Process data by format
    if (_incomingData.data[0] == 0xAB) {
        switch (_incomingData.data[4]) {
            // Handle reset
            case 0x23:
                if (configurationReceivedCallback != nullptr) {
                    configurationReceivedCallback(ConfigType::CF_RST, 0, 0);
                }
                break;
                
            // Handle health measurement request
            case 0x31:
                switch (_incomingData.data[5]) {
                    case 0x0A:
                        if (healthRequestCallback != nullptr) {
                            healthRequestCallback(HealthRequest::HR_HEART_RATE_MEASURE, _incomingData.data[6]);
                        }
                        break;
                    case 0x12:
                        if (healthRequestCallback != nullptr) {
                            healthRequestCallback(HealthRequest::HR_BLOOD_OXYGEN_MEASURE, _incomingData.data[6]);
                        }
                        break;
                    case 0x22:
                        if (healthRequestCallback != nullptr) {
                            healthRequestCallback(HealthRequest::HR_BLOOD_PRESSURE_MEASURE, _incomingData.data[6]);
                        }
                        break;
                }
                break;
                
            // Handle request to measure all health metrics
            case 0x32:
                if (healthRequestCallback != nullptr) {
                    healthRequestCallback(HealthRequest::HR_MEASURE_ALL, _incomingData.data[6]);
                }
                break;
                
            // Handle step data request
            case 0x51:
                if (_incomingData.data[5] == 0x80) {
                    if (healthRequestCallback != nullptr) {
                        healthRequestCallback(HealthRequest::HR_STEPS_RECORDS, true);
                    }
                }
                break;
                
            // Handle sleep data request
            case 0x52:
                if (_incomingData.data[5] == 0x80) {
                    if (healthRequestCallback != nullptr) {
                        healthRequestCallback(HealthRequest::HR_SLEEP_RECORDS, true);
                    }
                }
                break;
                
            // Handle find-device request
            case 0x71:
                if (configurationReceivedCallback != nullptr) {
                    configurationReceivedCallback(ConfigType::CF_FIND, 0, 0);
                }
                break;
                
            // Handle notification
            case 0x72: {
                int icon = _incomingData.data[6];
                int state = _incomingData.data[7];
                
                String message = "";
                for (int i = 8; i < _incomingData.length; i++) {
                    message += (char)_incomingData.data[i];
                }
                
                if (icon == 0x01) {
                    // Ring command
                    if (ringerAlertCallback != nullptr) {
                        ringerAlertCallback(message, true);
                    }
                    break;
                }
                if (icon == 0x02) {
                    // Cancel ring command
                    if (ringerAlertCallback != nullptr) {
                        ringerAlertCallback(message, false);
                    }
                    break;
                }
                if (state == 0x02) {
                    _notificationIndex++;
                    _notifications[_notificationIndex % NOTIF_SIZE].icon = icon;
                    _notifications[_notificationIndex % NOTIF_SIZE].app = "App"; // Could use appName(icon) if implemented
                    _notifications[_notificationIndex % NOTIF_SIZE].time = "Now";
                    splitTitle(message, _notifications[_notificationIndex % NOTIF_SIZE].title, _notifications[_notificationIndex % NOTIF_SIZE].message, icon);
                    
                    if (notificationReceivedCallback != nullptr) {
                        notificationReceivedCallback(_notifications[_notificationIndex % NOTIF_SIZE]);
                    }
                }
                break;
            }
            
            // Set alarm
            case 0x73: {
                uint8_t hour = _incomingData.data[8];
                uint8_t minute = _incomingData.data[9];
                uint8_t repeat = _incomingData.data[10];
                bool enabled = _incomingData.data[7];
                uint32_t index = (uint32_t)_incomingData.data[6];
                
                if (index < ALARM_SIZE) {
                    _alarms[index].hour = hour;
                    _alarms[index].minute = minute;
                    _alarms[index].repeat = repeat;
                    _alarms[index].enabled = enabled;
                    
                    if (configurationReceivedCallback != nullptr) {
                        uint32_t alarm = ((uint32_t)hour << 24) | ((uint32_t)minute << 16) | ((uint32_t)repeat << 8) | ((uint32_t)enabled);
                        configurationReceivedCallback(ConfigType::CF_ALARM, index, alarm);
                    }
                }
                break;
            }
            
            // Set 24h mode
            case 0x7C:
                _hour24 = (_incomingData.data[6] == 0);
                if (configurationReceivedCallback != nullptr) {
                    configurationReceivedCallback(ConfigType::CF_HR24, 0, (uint32_t)(_incomingData.data[6] == 0));
                }
                break;
                
            // Camera status
            case 0x79:
                _cameraReady = (_incomingData.data[6] == 1);
                if (configurationReceivedCallback != nullptr) {
                    configurationReceivedCallback(ConfigType::CF_CAMERA, 0, (uint32_t)_incomingData.data[6]);
                }
                break;
                
            // Phone battery info
            case 0x91:
                if (_incomingData.data[3] == 0xFF || _incomingData.data[3] == 0xFE) {
                    _phoneCharging = _incomingData.data[6] == 1;
                    _phoneBatteryLevel = _incomingData.data[7];
                    
                    if (configurationReceivedCallback != nullptr) {
                        configurationReceivedCallback(ConfigType::CF_PBAT, _phoneCharging ? 1 : 0, _phoneBatteryLevel);
                    }
                }
                break;
                
            // Update time
            case 0x93:
                if (_incomingData.length >= 14) {
                    this->setTime(_incomingData.data[13], _incomingData.data[12], _incomingData.data[11], 
                                 _incomingData.data[10], _incomingData.data[9], 
                                 _incomingData.data[7] * 256 + _incomingData.data[8]);
                    
                    if (configurationReceivedCallback != nullptr) {
                        configurationReceivedCallback(ConfigType::CF_TIME, 0, 0);
                    }
                }
                break;
                
            // App info
            case 0xCA:
                if (_incomingData.data[3] == 0xFE) {
                    _appCode = (_incomingData.data[6] * 256) + _incomingData.data[7];
                    _appVersion = "";
                    for (int i = 8; i < _incomingData.length; i++) {
                        if (_incomingData.data[i] != 0) {
                            _appVersion += (char)_incomingData.data[i];
                        }
                    }
                    
                    if (configurationReceivedCallback != nullptr) {
                        configurationReceivedCallback(ConfigType::CF_APP, _appCode, 0);
                    }
                    
                    _sendESP = true;
                }
                break;
                
            // Navigation icon data
            case 0xEE:
                if (_incomingData.data[3] == 0xFE) {
                    // Navigation icon data received
                    uint8_t pos = _incomingData.data[6];
                    uint32_t crc = uint32_t(_incomingData.data[7] << 24) | uint32_t(_incomingData.data[8] << 16) | 
                                  uint32_t(_incomingData.data[9] << 8) | uint32_t(_incomingData.data[10]);
                    
                    if (pos < 3 && (_incomingData.length - 11) <= 96) { // Ensure pos is valid and data length is reasonable
                        for (int i = 0; i < (_incomingData.length - 11); i++) {
                            if ((i + (96 * pos)) < ICON_DATA_SIZE) {
                                _navigation.icon[i + (96 * pos)] = _incomingData.data[11 + i];
                            }
                        }
                        
                        _navigation.hasIcon = true;
                        _navigation.iconCRC = crc;
                        
                        if (configurationReceivedCallback != nullptr) {
                            configurationReceivedCallback(ConfigType::CF_NAV_ICON, pos, crc);
                        }
                        
                        Serial.printf("Navigation Icon data, position: %d\n", pos);
                        Serial.printf("Icon CRC: 0x%08X\n", crc);
                        Serial.printf("Nav icon part %d received, CRC: 0x%08X\n", pos, crc);
                    }
                }
                break;
                
            // Navigation data
            case 0xEF:
                if (_incomingData.data[3] == 0xFE) {
                    // Navigation data received
                    if (_incomingData.data[5] == 0x00) {
                        // Navigation inactive
                        _navigation.active = false;
                        _navigation.eta = "Navigation";
                        _navigation.title = "Chronos";
                        _navigation.duration = "Inactive";
                        _navigation.distance = "";
                        _navigation.speed = "";
                        _navigation.directions = "Start navigation on Google maps";
                        _navigation.hasIcon = false;
                        _navigation.isNavigation = false;
                        _navigation.iconCRC = 0xFFFFFFFF;
                        
                        Serial.println("Navigation is inactive");
                    }
                    else if (_incomingData.data[5] == 0xFF) {
                        // Navigation disabled
                        _navigation.active = false;
                        _navigation.title = "Chronos";
                        _navigation.duration = "Disabled";
                        _navigation.distance = "";
                        _navigation.speed = "";
                        _navigation.eta = "Navigation";
                        _navigation.directions = "Check Chronos app settings";
                        _navigation.hasIcon = false;
                        _navigation.isNavigation = false;
                        _navigation.iconCRC = 0xFFFFFFFF;
                        
                        Serial.println("Navigation is disabled");
                    }
                    else if (_incomingData.data[5] == 0x80) {
                        _navigation.active = true;
                        _navigation.hasIcon = _incomingData.data[6] == 1;
                        _navigation.isNavigation = _incomingData.data[7] == 1;
                        _navigation.iconCRC = uint32_t(_incomingData.data[8] << 24) | uint32_t(_incomingData.data[9] << 16) | uint32_t(_incomingData.data[10] << 8) | uint32_t(_incomingData.data[11]);

                        int i = 12;
                        
                        // Title (field order matches the original library)
                        _navigation.title = "";
                        while (i < _incomingData.length && _incomingData.data[i] != 0)
                        {
                            _navigation.title += char(_incomingData.data[i]);
                            i++;
                        }
                        i++;

                        // Duration 
                        _navigation.duration = "";
                        while (i < _incomingData.length && _incomingData.data[i] != 0)
                        {
                            _navigation.duration += char(_incomingData.data[i]);
                            i++;
                        }
                        i++;

                        // Distance
                        _navigation.distance = "";
                        while (i < _incomingData.length && _incomingData.data[i] != 0)
                        {
                            _navigation.distance += char(_incomingData.data[i]);
                            i++;
                        }
                        i++;

                        // ETA
                        _navigation.eta = "";
                        while (i < _incomingData.length && _incomingData.data[i] != 0)
                        {
                            _navigation.eta += char(_incomingData.data[i]);
                            i++;
                        }
                        i++;

                        // Directions
                        _navigation.directions = "";
                        while (i < _incomingData.length && _incomingData.data[i] != 0)
                        {
                            _navigation.directions += char(_incomingData.data[i]);
                            i++;
                        }
                        i++;

                        // Speed
                        _navigation.speed = "";
                        while (i < _incomingData.length && _incomingData.data[i] != 0)
                        {
                            _navigation.speed += char(_incomingData.data[i]);
                            i++;
                        }
                        
                        Serial.println("Navigation data received - Raw fields:");
                        Serial.println("Title: [" + _navigation.title + "]");
                        Serial.println("Duration: [" + _navigation.duration + "]");
                        Serial.println("Distance: [" + _navigation.distance + "]");
                        Serial.println("ETA: [" + _navigation.eta + "]");
                        Serial.println("Directions: [" + _navigation.directions + "]");
                        Serial.println("Speed: [" + _navigation.speed + "]");
                        
                        printNavigationDetails();
                    }
                    
                    if (configurationReceivedCallback != nullptr) {
                        configurationReceivedCallback(ConfigType::CF_NAV_DATA, _navigation.active ? 1 : 0, 0);
                    }
                }
                break;
                
            // Default - handle other commands if needed
            default:
                break;
        }
    }
    else if (_incomingData.data[0] == 0xEA) {
        if (_incomingData.data[4] == 0x7E) {
            switch (_incomingData.data[5]) {
                case 0x01: {
                    // Weather city
                    String city = "";
                    for (int c = 7; c < _incomingData.length; c++) {
                        city += (char)_incomingData.data[c];
                    }
                    _weatherCity = city;
                    
                    if (configurationReceivedCallback != nullptr) {
                        configurationReceivedCallback(ConfigType::CF_WEATHER, 0, 1);
                    }
                    break;
                }
                case 0x02: {
                    // Hourly forecast
                    int size = _incomingData.data[6];
                    int hour = _incomingData.data[7];
                    
                    for (int z = 0; z < size; z++) {
                        if (hour + z >= FORECAST_SIZE) {
                            break;
                        }
                        
                        // Parse forecast data
                        // (Implementation depends on your forecast structure)
                    }
                    break;
                }
            }
        }
    }
    
    // Reset buffer after processing
    _incomingData.length = 0;
}

// Helper function to split notification title and content
void ChronosESP32Patched::splitTitle(const String &input, String &title, String &message, int icon) {
    int pos = input.indexOf('\n');
    if (pos > 0) {
        title = input.substring(0, pos);
        message = input.substring(pos + 1);
    } else {
        title = input;
        message = "";
    }
}

// Helper method for UTF-8 decoding - simplified
String ChronosESP32Patched::decodeUTF8(const uint8_t* data, int length) {
    if (length <= 0 || data == nullptr) {
        return "";
    }
    
    // ESP32 Arduino String already supports UTF-8, just convert uint8_t* to char*
    String result;
    result.reserve(length);
    
    for (int i = 0; i < length && data[i] != 0; i++) {
        result += (char)data[i];
    }
    
    return result;
}

void ChronosESP32Patched::sendInfo() {
    if (!_connected || !_subscribed) {
        return;
    }
    
    // Basic ESP32 info in the format expected by the Chronos app
    uint8_t infoCmd[] = {0xAB, 0x00, 0x11, 0xFF, 0x92, 0xC0, 
                         CHRONOSESP_VERSION_MAJOR, 
                         (CHRONOSESP_VERSION_MINOR * 10 + CHRONOSESP_VERSION_PATCH),
                         0x00, 0xFB, 0x1E, 0x40, 0xC0, 0x0E, 0x32, 0x28, 0x00, 0xE2, 
                         (uint8_t)_screenConf, 0x80};
    
    sendCommand(infoCmd, 20);
    Serial.println("Sent device info to Chronos app");
}

void ChronosESP32Patched::loop() {
    if (!_inited) {
        return;
    }
    
    unsigned long now = millis();
    
    // Handle timers and periodic tasks
    
    // Send battery info if it changed
    if (_batteryChanged && _connected && _subscribed) {
        sendBattery();
    }
    
    // Send ESP info periodically (every 60 seconds)
    static unsigned long lastInfoTime = 0;
    if (_connected && _subscribed && now - lastInfoTime > 60000) {
        lastInfoTime = now;
        sendInfo();
    }
    
    // Handle find-phone timer
    if (_findTimer.active && now - _findTimer.time > _findTimer.duration) {
        _findTimer.active = false;
        findPhone(false); // Automatically stop searching after the set duration
    }
}

Navigation ChronosESP32Patched::getNavigation() {
    // Print navigation details with our specific format
    printNavigationDetails();
    
    return _navigation;
}

bool ChronosESP32Patched::capturePhoto() {
    if (!_connected || !_subscribed) {
        return false;
    }
    
    uint8_t command[] = {0x9B, 0x00};
    sendCommand(command, sizeof(command));
    return true;
}

void ChronosESP32Patched::findPhone(bool state) {
    if (!_connected || !_subscribed) {
        return;
    }
    
    // Use the exact command structure from the original library
    uint8_t c = state ? 0x01 : 0x00;
    uint8_t findCmd[] = {0xAB, 0x00, 0x04, 0xFF, 0x7D, 0x80, c};
    sendCommand(findCmd, 7);
    
    // Set timer if currently searching
    if (state) {
        _findTimer.time = millis();
        _findTimer.active = true;
        _findTimer.duration = 30000; // 30 seconds, same as the original library
    } else {
        _findTimer.active = false;
    }
}

void ChronosESP32Patched::printNavigationDetails() {
    Serial.println("\n===== NAVIGATION DATA DETAILS =====");
    Serial.printf("Active: %s\n", _navigation.active ? "true" : "false");
    Serial.printf("Is Navigation: %s\n", _navigation.isNavigation ? "true" : "false");
    Serial.printf("Has Icon: %s\n", _navigation.hasIcon ? "true" : "false");
    
    // Display data fields in the correct order (original library)
    Serial.printf("Title: %s\n", _navigation.title.c_str());
    Serial.printf("Duration: %s\n", _navigation.duration.c_str());
    Serial.printf("Distance: %s\n", _navigation.distance.c_str());
    Serial.printf("ETA: %s\n", _navigation.eta.c_str());
    Serial.printf("Directions: %s\n", _navigation.directions.c_str());
    Serial.printf("Speed: %s\n", _navigation.speed.c_str());
    Serial.printf("Icon CRC: 0x%08X\n", _navigation.iconCRC);
    Serial.println("==================================");
}

void ChronosESP32Patched::setBattery(uint8_t level, bool charging) {
    _batteryLevel = level;
    _isCharging = charging;
    _batteryChanged = true;
    
    // If already connected, send battery info immediately
    if (_connected && _subscribed) {
        sendBattery();
    }
}

void ChronosESP32Patched::sendBattery() {
    if (!_connected || !_subscribed) {
        return;
    }
    
    // Use the exact command structure as in the original ChronosESP32 library
    uint8_t c = _isCharging ? 0x01 : 0x00;
    uint8_t batCmd[] = {0xAB, 0x00, 0x05, 0xFF, 0x91, 0x80, c, _batteryLevel};
    sendCommand(batCmd, 8);
    _batteryChanged = false;
}
