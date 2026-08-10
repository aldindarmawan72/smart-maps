#ifndef APP_STATE_MACHINE_H
#define APP_STATE_MACHINE_H

#include <Arduino.h>
#include "ChronosManager.h"

// 4 state utama aplikasi, sesuai PRD §4.
// NOTIF_OVERLAY sengaja TIDAK masuk sebagai state - itu layer terpisah
// yang muncul di atas state manapun (dibangun di Fase 7), bukan state
// yang menggantikan state dasar.
enum class AppState {
    BOOT_VIDEO,
    HOME,
    NAVIGATION,
    IDLE_VIDEO
};

class AppStateMachine {
private:
    AppState _currentState = AppState::BOOT_VIDEO;
    unsigned long _stateEnteredAt = 0;
    unsigned long _homeIdleStartedAt = 0;

    // 15 detik diam di HOME (tanpa navigasi aktif) -> pindah ke IDLE_VIDEO
    static const unsigned long HOME_IDLE_TIMEOUT_MS = 15000;

    // SEMENTARA untuk Fase 2: durasi "video" palsu, karena player video
    // asli baru dibangun di Fase 6. Nanti diganti sinyal "1 loop selesai"
    // dari video player yang sebenarnya, bukan timer tetap seperti ini.
    static const unsigned long PLACEHOLDER_VIDEO_DURATION_MS = 5000;

    void (*onStateChangeCallback)(AppState oldState, AppState newState) = nullptr;

    void changeState(AppState newState) {
        if (newState == _currentState) {
            return;
        }
        AppState old = _currentState;
        _currentState = newState;
        _stateEnteredAt = millis();

        if (newState == AppState::HOME) {
            _homeIdleStartedAt = millis();
        }

        Serial.printf("[StateMachine] %s -> %s\n", stateName(old), stateName(newState));

        if (onStateChangeCallback != nullptr) {
            onStateChangeCallback(old, newState);
        }
    }

public:
    static const char* stateName(AppState s) {
        switch (s) {
            case AppState::BOOT_VIDEO: return "BOOT_VIDEO";
            case AppState::HOME:       return "HOME";
            case AppState::NAVIGATION: return "NAVIGATION";
            case AppState::IDLE_VIDEO: return "IDLE_VIDEO";
        }
        return "UNKNOWN";
    }

    void setOnStateChange(void (*callback)(AppState, AppState)) {
        onStateChangeCallback = callback;
    }

    AppState getState() {
        return _currentState;
    }

    // Dipanggil tiap loop(). Membaca status BLE & navigasi dari
    // ChronosManager, lalu terapkan aturan transisi PRD §4.
    void update(ChronosManager& chronos) {
        bool connected = chronos.isConnected();
        bool navigating = chronos.isNavigating();
        unsigned long now = millis();

        // Aturan prioritas tertinggi: BLE disconnect dari state APA PUN -> BOOT_VIDEO
        if (!connected && _currentState != AppState::BOOT_VIDEO) {
            changeState(AppState::BOOT_VIDEO);
            return;
        }

        switch (_currentState) {
            case AppState::BOOT_VIDEO:
                if (connected) {
                    changeState(AppState::HOME);
                }
                break;

            case AppState::HOME:
                if (navigating) {
                    changeState(AppState::NAVIGATION);
                } else if (now - _homeIdleStartedAt >= HOME_IDLE_TIMEOUT_MS) {
                    changeState(AppState::IDLE_VIDEO);
                }
                break;

            case AppState::NAVIGATION:
                if (!navigating) {
                    changeState(AppState::HOME);
                }
                break;

            case AppState::IDLE_VIDEO:
                // Prioritas tertinggi: navigasi aktif interupsi LANGSUNG,
                // tanpa menunggu "video" (placeholder timer) selesai.
                if (navigating) {
                    changeState(AppState::NAVIGATION);
                } else if (now - _stateEnteredAt >= PLACEHOLDER_VIDEO_DURATION_MS) {
                    changeState(AppState::HOME);
                }
                break;
        }
    }

    static AppStateMachine& getInstance() {
        static AppStateMachine instance;
        return instance;
    }
};

#endif // APP_STATE_MACHINE_H
