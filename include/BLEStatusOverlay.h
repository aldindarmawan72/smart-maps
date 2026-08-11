#ifndef BLE_STATUS_OVERLAY_H
#define BLE_STATUS_OVERLAY_H

#include <Arduino.h>
#include "LGFX_Config.h"

// Kelas untuk menampilkan status koneksi BLE
class BLEStatusOverlay {
private:
  LGFX* _tft = nullptr;
  bool _isShowing = false;
  unsigned long _showStartTime = 0;
  static constexpr unsigned long SHOW_DURATION = 2000; // 2 detik

public:
  BLEStatusOverlay() {}

  void init(LGFX* tft) {
    _tft = tft;
  }

  // Tampilkan notifikasi BLE connected
  void showConnected() {
    if (!_tft) return;

    // Tampilkan overlay notifikasi di pojok kanan atas
    int x = _tft->width() - 100;
    int y = 5;
    int width = 95;
    int height = 30;

    _tft->fillRoundRect(x, y, width, height, 5, TFT_DARKGREEN);
    _tft->drawRoundRect(x, y, width, height, 5, TFT_GREEN);

    _tft->setTextColor(TFT_WHITE);
    _tft->setTextSize(1);
    _tft->setCursor(x + 10, y + 10);
    _tft->println("BLE TERHUBUNG");

    _isShowing = true;
    _showStartTime = millis();
  }

  // Tampilkan notifikasi BLE disconnected
  void showDisconnected() {
    if (!_tft) return;

    // Tampilkan overlay notifikasi di pojok kanan atas
    int x = _tft->width() - 100;
    int y = 5;
    int width = 95;
    int height = 30;

    _tft->fillRoundRect(x, y, width, height, 5, TFT_MAROON);
    _tft->drawRoundRect(x, y, width, height, 5, TFT_RED);

    _tft->setTextColor(TFT_WHITE);
    _tft->setTextSize(1);
    _tft->setCursor(x + 10, y + 10);
    _tft->println("BLE TERPUTUS");

    _isShowing = true;
    _showStartTime = millis();
  }

  // Update dan cek apakah notifikasi perlu dimatikan
  void update() {
    if (_isShowing && millis() - _showStartTime >= SHOW_DURATION) {
      _isShowing = false;
      // Tidak perlu hapus notifikasi karena layar akan di-update oleh video atau navigasi
    }
  }

  bool isShowing() {
    return _isShowing;
  }

  static BLEStatusOverlay& getInstance() {
    static BLEStatusOverlay instance;
    return instance;
  }
};

#endif // BLE_STATUS_OVERLAY_H
