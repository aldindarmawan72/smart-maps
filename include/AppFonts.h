#ifndef APP_FONTS_H
#define APP_FONTS_H

#include <Arduino.h>
#include <lvgl.h>          // Include LVGL wrapper first
#include "fonts/local_fonts.h"

// Warna-warna untuk UI
namespace NavColors {
    static const uint32_t BackgroundHex = 0x000000;  // Hitam
    static const uint32_t ForegroundHex = 0xFFFFFF;  // Putih
    static const uint32_t AccentHex = 0x00FF00;      // Hijau
    static const uint32_t SecondaryHex = 0x0000FF;   // Biru
    static const uint32_t AttentionHex = 0xFF0000;   // Merah
    static const uint32_t InfoHex = 0xFFFF00;        // Kuning
}

// Kelas pengelola font aplikasi
class AppFonts {
private:
    static lv_font_t* _normalFont;
    static lv_font_t* _boldFont;
    static lv_font_t* _semiboldFont;
    static lv_font_t* _numberBoldFont;

public:
    // Inisialisasi font-font
    static void init() {
        // Gunakan font yang sudah disiapkan
        _normalFont = get_montserrat_24();
        _boldFont = get_montserrat_bold_32();
        _semiboldFont = get_montserrat_semibold_28();
        _numberBoldFont = get_montserrat_number_bold_48();

        // Print info inisialisasi font
        Serial.println("App fonts initialized");
        Serial.println("Using UTF-8 encoding for text");
    }

    // Ambil font normal
    static lv_font_t* getNormalFont() {
        return _normalFont;
    }

    // Ambil font bold
    static lv_font_t* getBoldFont() {
        return _boldFont;
    }

    // Ambil font semi-bold
    static lv_font_t* getSemiboldFont() {
        return _semiboldFont;
    }

    // Ambil font angka bold
    static lv_font_t* getNumberBoldFont() {
        return _numberBoldFont;
    }

    // Buat objek text LVGL dengan font aplikasi
    static lv_obj_t* createText(lv_obj_t* parent, const char* text, lv_font_t* font, lv_color_t color, lv_align_t align, lv_coord_t x, lv_coord_t y) {
        lv_obj_t* label = lv_label_create(parent);

        // Set text
        lv_label_set_text(label, text);

        // Pastikan pakai font Montserrat dengan encoding penuh
        lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, color, LV_PART_MAIN);

        // Set posisi label
        lv_obj_align(label, align, x, y);

        return label;
    }
};

// Definisi variabel statis
lv_font_t* AppFonts::_normalFont = nullptr;
lv_font_t* AppFonts::_boldFont = nullptr;
lv_font_t* AppFonts::_semiboldFont = nullptr;
lv_font_t* AppFonts::_numberBoldFont = nullptr;

#endif // APP_FONTS_H
