#ifndef CHRONOS_TYPES_H
#define CHRONOS_TYPES_H

#include <Arduino.h>

// We'll use our own navigation type with a different name
// DO NOT redefine Navigation with a macro as it causes conflicts
// Instead, use a different name for our own structure

// Our custom Navigation type that won't conflict with library
struct AppNavigation {
    bool active = false;
    bool hasIcon = false;
    bool isNavigation = false;
    uint32_t iconCRC = 0;
    String eta = "";         // Estimated time of arrival
    String title = "";       // Current street/place name
    String duration = "";    // Remaining trip duration
    String distance = "";    // Distance to destination
    String speed = "";       // Current speed
    String directions = "";  // Next turn instruction
    uint8_t icon[384];       // 96*4 bytes for icon data

    // Default constructor
    AppNavigation() {
        active = false;
        hasIcon = false;
        isNavigation = false;
        iconCRC = 0xFFFFFFFF;
        eta = "";
        title = "";
        duration = "";
        distance = "";
        speed = "";
        directions = "";
        memset(icon, 0, sizeof(icon));
    }
};

// Enums to match the library's configuration types
// Use more specific name to avoid conflicts
enum class AppConfigType {
    CF_TIME,
    CF_NAV_DATA,
    CF_NAV_ICON,
    CF_PBAT,
    CF_ALARM,
    CF_USER,
    CF_SED,
    CF_QUIET,
    CF_RTW,
    CF_HOURLY,
    CF_CAMERA,
    CF_LANG,
    CF_HR24,
    CF_SLEEP,
    CF_APP,
    CF_CONTACT,
    CF_QR,
    CF_FONT,
    CF_RST
};

#endif // CHRONOS_TYPES_H
