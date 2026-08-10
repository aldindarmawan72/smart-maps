#ifndef LVGL_CONFIG_H
#define LVGL_CONFIG_H

#include <lvgl.h>
#include <Arduino.h>
#include "LGFX_Config.h"  // Konfigurasi LovyanGFX (driver ST7789)


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

        // CATATAN PIN: BLK (backlight) panel ini diasumsikan terhubung
        // langsung ke 3V3 (always-on), BUKAN dikontrol lewat GPIO.
        // GPIO7 sudah dipakai sebagai pin DC oleh LGFX_Config.h (SPI),
        // jadi TIDAK BOLEH dipakai ulang untuk backlight - itu akan
        // bentrok dengan jalur sinyal DC dan bisa bikin layar glitch.
        // Kalau nanti mau kontrol brightness lewat GPIO, pakai pin
        // bebas lain (misal GPIO2/GPIO3) dan update setBacklight() di bawah.

        // Inisialisasi LVGL
        lv_init();

        // Alokasi buffer render (20 baris, double buffer)
        _buf1 = (lv_color_t *)heap_caps_malloc(_screenWidth * 20 * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        _buf2 = (lv_color_t *)heap_caps_malloc(_screenWidth * 20 * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

        if (!_buf1 || !_buf2) {
            Serial.println("Error allocating display buffer!");
            return;
        }

        lv_disp_draw_buf_init(&_draw_buf, _buf1, _buf2, _screenWidth * 20);

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

    // Backlight belum dikontrol lewat GPIO (lihat catatan di init()).
    // Method ini sengaja jadi no-op sampai BLK dikabel ke pin bebas.
    void setBacklight(bool on) {
        // TODO: implementasikan setelah BLK dikabel ke GPIO bebas (bukan GPIO7)
    }

    static LVGL_Display& getInstance() {
        static LVGL_Display instance;
        return instance;
    }
};

#endif // LVGL_CONFIG_H
