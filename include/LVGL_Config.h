#ifndef LVGL_CONFIG_H
#define LVGL_CONFIG_H

#include <Arduino.h>
#include "LGFX_Config.h"  // Konfigurasi LovyanGFX
#include <lvgl.h>         // Include LVGL sebelum file lain yang bergantung padanya

class LVGL_Display {
private:
    LGFX _tft;
    lv_disp_draw_buf_t _draw_buf;
    lv_color_t *_buf1 = nullptr;
    lv_color_t *_buf2 = nullptr;
    lv_disp_drv_t _disp_drv;
    static const uint32_t _screenWidth = 240;
    static const uint32_t _screenHeight = 240;

    // Callback LVGL untuk menggambar ke layar fisik
    static void _lvgl_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
        LVGL_Display *display = (LVGL_Display *)disp->user_data;

        uint32_t w = (area->x2 - area->x1 + 1);
        uint32_t h = (area->y2 - area->y1 + 1);

        display->_tft.startWrite();
        display->_tft.setAddrWindow(area->x1, area->y1, w, h);
        display->_tft.writePixels((uint16_t *)color_p, w * h);
        display->_tft.endWrite();

        lv_disp_flush_ready(disp);
    }

public:
    LVGL_Display() {}

    void init() {
        Serial.println("Initializing LVGL Display...");

        // Inisialisasi LovyanGFX
        _tft.init();
        _tft.setRotation(0);
        _tft.setBrightness(255);

        // Konfigurasi backlight untuk ESP32-C3
        pinMode(7, OUTPUT);
        digitalWrite(7, HIGH); // BLK ON

        // Inisialisasi LVGL
        lv_init();

        // Pastikan animation aktif
        #if LV_USE_ANIMATION
        Serial.println("LVGL animations are enabled");
        #else
        Serial.println("WARNING: LVGL animations are disabled!");
        #endif

        // Alokasi buffer render
        _buf1 = (lv_color_t *)heap_caps_malloc(_screenWidth * 20 * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        _buf2 = (lv_color_t *)heap_caps_malloc(_screenWidth * 20 * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

        if (!_buf1 || !_buf2) {
            Serial.println("Error allocating display buffer!");
            return;
        }

        // Inisialisasi buffer gambar
        lv_disp_draw_buf_init(&_draw_buf, _buf1, _buf2, _screenWidth * 20);

        // Inisialisasi driver display
        lv_disp_drv_init(&_disp_drv);
        _disp_drv.hor_res = _screenWidth;
        _disp_drv.ver_res = _screenHeight;
        _disp_drv.flush_cb = _lvgl_flush_cb;
        _disp_drv.draw_buf = &_draw_buf;
        _disp_drv.user_data = this;
        lv_disp_drv_register(&_disp_drv);

        Serial.println("LVGL Display initialized");
    }

    void update() {
        static uint32_t last_tick = 0;
        uint32_t current_tick = millis();

        uint32_t tick_diff = current_tick - last_tick;
        if (tick_diff > 0) {
            lv_tick_inc(tick_diff);
            last_tick = current_tick;
        }

        lv_timer_handler();
    }

    LGFX* getTft() {
        return &_tft;
    }

    void setBacklight(bool on) {
        digitalWrite(7, on ? HIGH : LOW);
    }

    static LVGL_Display& getInstance() {
        static LVGL_Display instance;
        return instance;
    }
};

#endif // LVGL_CONFIG_H
