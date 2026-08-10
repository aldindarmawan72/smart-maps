#ifndef CHRONOS_MANAGER_H
#define CHRONOS_MANAGER_H

#include <Arduino.h>
#include "Config.h"
// Include our types first
#include "ChronosTypes.h"
// Then include ChronosESP32 adapter
#include "ChronosESP32Adapter.h"

// Forward declarations
class LGFX;
// (Navigation sudah didefinisikan lengkap lewat include di atas,
// jadi tidak perlu forward-declare ulang di sini)

// Manager class for interacting with the Chronos app using the ChronosESP32 library
class ChronosManager {
private:
    ChronosESP32Adapter* chronos = nullptr;
    LGFX* _tft = nullptr;
    bool _isConnected = false;
    bool _isNavigating = false;
    String _address;
    
    // Current navigation data
    AppNavigation _navData;
    
    // Callback when a navigation icon is received
    static void iconCallbackHandler(uint8_t icon, String data) {
        ChronosManager& instance = getInstance();
        
        Serial.println("Received navigation icon: " + String(icon));
        instance._navData.hasIcon = true;
        instance.handleConfigChange(ConfigType::CF_NAV_ICON, icon, 0);
    }
    
    // Callback when connection state changes
    static void connectCallbackHandler(bool connected) {
        ChronosManager& instance = getInstance();
        instance._isConnected = connected;
        
        Serial.println(connected ? "BLE connected to Chronos app" : "BLE disconnected from Chronos app");
        
        // Call the connection-change handler
        instance.handleConnectionChange(connected);
    }
    
    // Callback when config info is received - designed to match the example exactly
    static void configCallbackHandler(ConfigType config, uint32_t value1, uint32_t value2) {
        ChronosManager& instance = getInstance();
        
        switch (config)
        {
        case ConfigType::CF_NAV_DATA:
            Serial.print("Navigation state: ");
            Serial.println(value1 ? "Active" : "Inactive");
            
            // Mark that the navigation state has changed
            instance._isNavigating = value1 == 1;
            instance._navData.active = value1 == 1;
            
            // If navigation is active, fetch the navigation data
            if (value1) {
                if (instance.chronos != nullptr) {
                    // Get navigation info from the ChronosESP32 object
                    Navigation nav = instance.chronos->getNavigation();
                    
                    // Update navigation data in the new field order
                    instance._navData.distance = nav.distance;
                    instance._navData.duration = nav.duration;
                    instance._navData.eta = nav.eta;
                    instance._navData.directions = nav.directions;
                    instance._navData.title = nav.title;
                    instance._navData.speed = nav.speed;
                    instance._navData.hasIcon = nav.hasIcon;
                    instance._navData.isNavigation = nav.isNavigation;
                    
                    // Copy icon data if present
                    if (nav.hasIcon) {
                        memcpy(instance._navData.icon, nav.icon, ICON_DATA_SIZE);
                        instance._navData.iconCRC = nav.iconCRC;
                    }
                    
                    // Print debug info in the new order
                    Serial.println(nav.distance);
                    Serial.println(nav.duration);
                    Serial.println(nav.eta);
                    Serial.println(nav.directions);
                    Serial.println(nav.title);
                    Serial.println(nav.speed);
                }
            }
            break;
            
        case ConfigType::CF_NAV_ICON:
            Serial.print("CF_NAV_ICON Navigation Icon data, position: ");
            Serial.println(value1);
            Serial.print("Icon CRC: ");
            Serial.printf("0x%08X\n", value2);
            
            // Handle when icon data is received
            if (instance.chronos != nullptr) {
                Navigation nav = instance.chronos->getNavigation();
                instance._navData.hasIcon = nav.hasIcon;
                instance._navData.iconCRC = nav.iconCRC;
                
                // Copy icon data from Navigation to AppNavigation
                if (nav.hasIcon) {
                    // Copy the entire icon data
                    memcpy(instance._navData.icon, nav.icon, ICON_DATA_SIZE);
                }
            }
            break;
            
        case ConfigType::CF_TIME:
            Serial.println("Received time sync from app");
            break;
            
        default:
            Serial.printf("Received config type: %d, values: %u, %u\n", 
                         (int)config, value1, value2);
            break;
        }
    }
    
    // Callback when a notification is received
    static void notificationCallbackHandler(Notification notification) {
        Serial.print("Notification received at ");
        Serial.println(notification.time);
        Serial.print("From: ");
        Serial.print(notification.app);
        Serial.print("\tIcon: ");
        Serial.println(notification.icon);
        Serial.println(notification.title);
        Serial.println(notification.message);
    }
    
    // Callback when time info is received
    static void timeCallbackHandler(uint8_t hour, uint8_t minute, uint8_t second, uint8_t day, uint8_t month, uint16_t year) {
        ChronosManager& instance = getInstance();
        
        Serial.printf("Received time: %02d:%02d:%02d %02d/%02d/%04d\n", 
                     hour, minute, second, day, month, year);
        
        instance.handleConfigChange(ConfigType::CF_TIME, 0, 0);
    }

public:
    // Get the underlying Chronos object
    ChronosESP32Patched& getChronos() {
        return chronos->getChronos();
    }
    ChronosManager() {
        // Initialize navigation data with default values
        _navData.active = false;
        _navData.eta = "Navigation";
        _navData.title = "Smart Maps";
        _navData.duration = "";
        _navData.distance = "";
        _navData.speed = "";
        _navData.directions = "";
        _navData.hasIcon = false;
        _navData.isNavigation = false;
        _navData.iconCRC = 0xFFFFFFFF;
        
        // Create the adapter object for ChronosESP32
        chronos = new ChronosESP32Adapter();
    }
    
    ~ChronosManager() {
        if (chronos != nullptr) {
            delete chronos;
            chronos = nullptr;
        }
    }
    
    void init(LGFX* tft) {
        _tft = tft;
        
        Serial.println("Initializing Chronos Manager with ChronosESP32 library...");
        
        // Register all callbacks before init, matching the reference example
        if (chronos != nullptr) {
            // Register callbacks exactly as in the example file
            chronos->setConnectionCallback(connectCallbackHandler);
            chronos->setNotificationCallback(notificationCallbackHandler);
            chronos->setConfigurationCallback(configCallbackHandler);
            
            Serial.println("All callbacks registered successfully");
        }
        
        // Set the BLE device name BEFORE begin() - begin() starts advertising
        // immediately using whatever name is set at that point. Without this,
        // the device falls back to the library's default placeholder name.
        chronos->setName("Smart Maps Watch");

        // Initialize BLE with the device name set above ("Smart Maps Watch")
        if (chronos->begin(false)) {
            Serial.println("ChronosESP32 initialized successfully");
            
            // Save BLE address
            _address = NimBLEDevice::getAddress().toString().c_str();
            
            // Set battery level
            chronos->setBattery(100, false);
            
            Serial.println("Chronos Manager initialized");
            Serial.println("BLE address: " + _address);
        } else {
            Serial.println("Failed to initialize ChronosESP32");
        }
    }
    
    void update() {
        // Call ChronosESP32's loop() to handle internal tasks, as in the example
        if (chronos != nullptr) {
            chronos->loop();
        }
        
        // Check BLE connection state
        bool currentConnected = isConnected();
        static bool lastConnected = false;
        
        // Handle connection state change
        if (currentConnected != lastConnected) {
            if (currentConnected) {
                // Just connected
                handleConnectionChange(true);
                Serial.println("ChronosManager: BLE connected");
            } else {
                // Just disconnected
                handleConnectionChange(false);
                Serial.println("ChronosManager: BLE disconnected");
            }
            lastConnected = currentConnected;
        }
        
        // Print current navigation info to Serial every 10 seconds
        static unsigned long lastNavLog = 0;
        if (_isNavigating && millis() - lastNavLog > 10000) {
            lastNavLog = millis();
            Serial.println("Navigation state: " + String(_isNavigating ? "ACTIVE" : "INACTIVE"));
            
            if (_isNavigating) {
                Serial.println("\n===== CURRENT NAVIGATION DATA =====");
                Serial.println("- Active: " + String(_navData.active ? "true" : "false"));
                Serial.println("- HasIcon: " + String(_navData.hasIcon ? "true" : "false"));
                Serial.println("- IconCRC: 0x" + String(_navData.iconCRC, HEX));
                Serial.println("- Title: " + _navData.title);
                Serial.println("- Distance: " + _navData.distance);
                Serial.println("- Duration: " + _navData.duration);
                Serial.println("- ETA: " + _navData.eta);
                Serial.println("- Directions: " + _navData.directions);
                Serial.println("=================================");
            }
        }
    }
    
    bool isConnected() {
        if (chronos != nullptr) {
            return chronos->isConnected();
        }
        return _isConnected;
    }
    
    bool isNavigating() {
        return _isNavigating;
    }
    
    AppNavigation getNavData() {
        return _navData;
    }
    
    // Send a custom command to the Chronos device
    void sendCommand(uint8_t* command, size_t length) {
        if (chronos != nullptr) {
            Serial.println("Sending custom command to Chronos device");
            for (size_t i = 0; i < length; i++) {
                Serial.printf("0x%02X ", command[i]);
            }
            Serial.println();
            
            // Use the adapter to send the command
            chronos->sendCommand(command, length);
        } else {
            Serial.println("Failed to send command: Chronos adapter not initialized");
        }
    }
    
    // Handle connection state change
    void handleConnectionChange(bool connected) {
        if (connected) {
            Serial.println("BLE connected to Chronos app");
        } else {
            Serial.println("BLE disconnected from Chronos app");
            _isNavigating = false;
            _navData.active = false;
        }
    }
    
    // Convert from ConfigType to AppConfigType for internal use
    AppConfigType convertConfigType(ConfigType config) {
        switch(config) {
            case ConfigType::CF_TIME:
                return AppConfigType::CF_TIME;
            case ConfigType::CF_NAV_DATA:
                return AppConfigType::CF_NAV_DATA;
            case ConfigType::CF_NAV_ICON:
                return AppConfigType::CF_NAV_ICON;
            case ConfigType::CF_PBAT:
                return AppConfigType::CF_PBAT;
            case ConfigType::CF_ALARM:
                return AppConfigType::CF_ALARM;
            case ConfigType::CF_USER:
                return AppConfigType::CF_USER;
            case ConfigType::CF_SED:
                return AppConfigType::CF_SED;
            case ConfigType::CF_QUIET:
                return AppConfigType::CF_QUIET;
            case ConfigType::CF_RTW:
                return AppConfigType::CF_RTW;
            case ConfigType::CF_HOURLY:
                return AppConfigType::CF_HOURLY;
            case ConfigType::CF_CAMERA:
                return AppConfigType::CF_CAMERA;
            case ConfigType::CF_LANG:
                return AppConfigType::CF_LANG;
            case ConfigType::CF_HR24:
                return AppConfigType::CF_HR24;
            case ConfigType::CF_SLEEP:
                return AppConfigType::CF_SLEEP;
            case ConfigType::CF_APP:
                return AppConfigType::CF_APP;
            case ConfigType::CF_CONTACT:
                return AppConfigType::CF_CONTACT;
            case ConfigType::CF_QR:
                return AppConfigType::CF_QR;
            case ConfigType::CF_FONT:
                return AppConfigType::CF_FONT;
            case ConfigType::CF_RST:
                return AppConfigType::CF_RST;
            default:
                // Default case, return CF_TIME
                return AppConfigType::CF_TIME;
        }
    }
    
    // Handle when config is received from the app
    void handleConfigChange(ConfigType config, uint32_t value1, uint32_t value2) {
        // Convert to AppConfigType
        AppConfigType appConfig = convertConfigType(config);
        
        switch(appConfig) {
            case AppConfigType::CF_TIME:
                Serial.println("Received time sync from app");
                break;
                
            case AppConfigType::CF_NAV_DATA:
                _isNavigating = value1 == 1;
                Serial.printf("Navigation status changed: %s\n", _isNavigating ? "Active" : "Inactive");
                break;
                
            case AppConfigType::CF_NAV_ICON:
                Serial.printf("Received navigation icon: %d\n", (int)value1);
                break;
                
            default:
                break;
        }
    }
    
    // Restart BLE advertising to restore connection
    void restartAdvertising() {
        if (chronos != nullptr) {
            Serial.println("Restarting BLE advertising");
            chronos->restart();
        }
    }
    
    // Debug function to display current state
    void debugCurrentState() {
        Serial.println("\n============ CHRONOS BLE STATUS ============");
        Serial.printf("Connected: %s\n", isConnected() ? "YES" : "NO");
        Serial.printf("Navigation Active: %s\n", _isNavigating ? "YES" : "NO");
        Serial.printf("BLE Address: %s\n", _address.c_str());
        
        // If connected, try to fetch navigation data from the library
        if (chronos != nullptr && chronos->isConnected()) {
            Navigation nav = chronos->getNavigation();
            
            // Update data from the library in the new field order
            _navData.active = nav.active;
            _navData.isNavigation = nav.isNavigation;
            _navData.hasIcon = nav.hasIcon;
            _navData.iconCRC = nav.iconCRC;
            _navData.distance = nav.distance;
            _navData.duration = nav.duration;
            _navData.eta = nav.eta;
            _navData.directions = nav.directions;
            _navData.title = nav.title;
            _navData.speed = nav.speed;
            _isNavigating = nav.active;
            
            // Copy icon data if present
            if (nav.hasIcon) {
                memcpy(_navData.icon, nav.icon, ICON_DATA_SIZE);
                Serial.println("Copied icon data from library cache");
            }
            
            Serial.println("\nFetched navigation data from library cache");
        }
        
        Serial.println("\n----- NAVIGATION DATA -----");
        Serial.printf("Active: %s\n", _navData.active ? "YES" : "NO");
        Serial.printf("HasIcon: %s\n", _navData.hasIcon ? "YES" : "NO");
        Serial.printf("IconCRC: 0x%08X\n", _navData.iconCRC);
        Serial.printf("Title: %s\n", _navData.title.c_str());
        Serial.printf("Distance: %s\n", _navData.distance.c_str());
        Serial.printf("Duration: %s\n", _navData.duration.c_str());
        Serial.printf("ETA: %s\n", _navData.eta.c_str());
        Serial.printf("Directions: %s\n", _navData.directions.c_str());
        Serial.println("===========================================");
    }
    
    static ChronosManager& getInstance() {
        static ChronosManager instance;
        return instance;
    }
};

#endif // CHRONOS_MANAGER_H
