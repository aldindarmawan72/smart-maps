#ifndef NAVIGATION_SCREEN_LVGL_H
#define NAVIGATION_SCREEN_LVGL_H

#include <Arduino.h>
#include <lvgl.h>
#include "LVGL_Config.h"
#include "AppFonts.h"
#include "ChronosTypes.h"
#include "ChronosManager.h"
#include "ESP32Time.h"
#include "bg.h" // Tambahkan include untuk menggunakan gambar latar

// Ukuran data ikon petunjuk arah dari aplikasi Chronos
#define ICON_DATA_SIZE 288 // 48x48 piksel, 1 bit per piksel = 48*48/8 = 288 byte

// Fungsi untuk mengonversi bitmap 1-bit ke RGB565
void convert1BitBitmapToRgb565(void* dst, const void* src, uint16_t width, uint16_t height, uint16_t color, uint16_t bgColor, bool invert = false) {
    uint16_t* d      = (uint16_t*)dst;
    const uint8_t* s = (const uint8_t*)src;

    auto activeColor   = invert ? bgColor : color;
    auto inactiveColor = invert ? color : bgColor;

    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            if (s[(y * width + x) / 8] & (1 << (7 - x % 8))) {
                d[y * width + x] = activeColor;
            } else {
                d[y * width + x] = inactiveColor;
            }
        }
    }
}

// Kelas untuk menampilkan layar navigasi menggunakan LVGL
class NavigationScreenLVGL {
private:
    lv_obj_t* _screen = nullptr;
    lv_obj_t* _navIcon = nullptr;
    lv_obj_t* _directionLabel = nullptr;
    lv_obj_t* _distanceLabel = nullptr;
    lv_obj_t* _titleLabel = nullptr;     // Ganti _etaLabel dengan _titleLabel
    lv_obj_t* _timeLabel = nullptr;
    lv_obj_t* _speedLabel = nullptr;     // Tambahkan label untuk menampilkan kecepatan
    lv_obj_t* _durationLabel = nullptr;  // Tambahkan label untuk menampilkan waktu perjalanan
    lv_obj_t* _bgImage = nullptr; // Tambahkan objek untuk gambar latar
    lv_obj_t* _defaultMessageLabel = nullptr; // Label menampilkan "Mulai navigasi di Google maps" saat tidak aktif
    lv_obj_t* _distanceContainer = nullptr; // Wadah untuk jarak dengan latar belakang kuning
    lv_obj_t* _defaultMessageContainer = nullptr; // Wadah latar belakang untuk pesan default

    // Inactive dashboard components
    lv_obj_t* _inactiveTimeLabel = nullptr;
    lv_obj_t* _inactiveDateLabel = nullptr;
    lv_obj_t* _inactiveBleLabel = nullptr;

    // Ikon navigasi
    lv_img_dsc_t _navIconDesc = {};
    uint8_t* _iconData = nullptr;
    uint8_t _iconBuffer[ICON_DATA_SIZE]; // Buffer untuk menyimpan salinan data ikon
    bool _hasValidIcon = false;
    uint32_t _iconCRC = 0;

    // Data navigasi
    AppNavigation _navData;

    // Gambar langsung bitmap 1-bit menggunakan API LVGL
    void drawNavIconDirectly() {
        if (!_hasValidIcon || _iconData == nullptr || _screen == nullptr) {
        //
            return;
        }

        // Hapus ikon lama jika ada
        if (_navIcon != nullptr) {
            lv_obj_del(_navIcon);
            _navIcon = nullptr;
        }

        // Hapus memori untuk deskriptor lama jika ada
        if (_navIconDesc.data != nullptr && _navIconDesc.data != _iconData) {
            _navIconDesc.data = nullptr;
        }

        // Buat larik piksel baru dalam format TRUE_COLOR
        static lv_color_t pixels[48 * 48];

        uint16_t activeColor = 0xFFFF;   // Putih
        uint16_t bgColor = 0x0000;       // Hitam

        // Konversikan bitmap 1-bit ke RGB565 menggunakan fungsi asli
        convert1BitBitmapToRgb565(pixels, _iconData, 48, 48, activeColor, bgColor, false);

        // Buat deskriptor dari data piksel TRUE_COLOR
        _navIconDesc.header.cf = LV_IMG_CF_TRUE_COLOR;
        _navIconDesc.header.always_zero = 0;
        _navIconDesc.header.reserved = 0;
        _navIconDesc.header.w = 48;
        _navIconDesc.header.h = 48;
        _navIconDesc.data_size = 48 * 48 * sizeof(lv_color_t);
        _navIconDesc.data = (const uint8_t*)pixels;

        // Buat objek gambar
        _navIcon = lv_img_create(_screen);

        // Terapkan sumber gambar
        lv_img_set_src(_navIcon, &_navIconDesc);

        // Tambahkan efek zoom untuk menyorot ikon (zoom 2x)
        lv_img_set_zoom(_navIcon, 512);

        // Pusatkan, atur asal 35 piksel dari tepi atas
        lv_obj_align(_navIcon, LV_ALIGN_TOP_MID, 0, 35);
    }

public:
    NavigationScreenLVGL() : _iconData(_iconBuffer), _hasValidIcon(false), _iconCRC(0) {
        // Inisialisasi buffer ikon dengan nilai 0
        memset(_iconBuffer, 0, ICON_DATA_SIZE);
    }

    ~NavigationScreenLVGL() {
        // Jangan kosongkan memori statis atau penunjuk langsung
        // LVGL menggunakan penunjuk ke data piksel di buffer statis
        _navIconDesc.data = nullptr;

        // Hapus objek LVGL jika masih ada
        if (_navIcon) {
            lv_obj_del(_navIcon);
            _navIcon = nullptr;
        }
    }

    // Periksa apakah layar telah dibuat dan siap untuk ditampilkan
    bool isScreenReady() {
        return _screen != nullptr;
    }

    // Perbarui data navigasi
    void updateNavigation(const AppNavigation &navData) {
        // Cek status active sebelum update
        bool wasActive = _navData.active && _navData.isNavigation;
        bool willBeActive = navData.active && navData.isNavigation;

        // Catat perubahan status
        if (wasActive != willBeActive) {
            Serial.printf("Navigation state changing: %s -> %s\n", 
                         wasActive ? "active" : "inactive", 
                         willBeActive ? "active" : "inactive");

            // Jika beralih dari aktif ke tidak aktif, setel ulang UI sepenuhnya
            if (wasActive && !willBeActive) {
                Serial.println("Resetting navigation UI state completely");

                // Setel ulang konten label
                if (_directionLabel) lv_label_set_text(_directionLabel, "");
                if (_titleLabel) lv_label_set_text(_titleLabel, "");
                if (_distanceLabel) lv_label_set_text(_distanceLabel, "0 km");
                if (_speedLabel) lv_label_set_text(_speedLabel, "");
            }

            // Jika beralih dari tidak aktif ke aktif, pastikan wadah jarak memiliki ukuran yang sesuai
            if (!wasActive && willBeActive) {
                Serial.println("Switching from inactive to active - ensuring container sizes");

                // Pastikan wadah ditampilkan sebelum memperbarui ukuran
                lv_obj_clear_flag(_distanceContainer, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(_distanceLabel, LV_OBJ_FLAG_HIDDEN);

                // Ubah ukuran wadah sesuai dengan konten
                lv_obj_set_width(_distanceContainer, LV_SIZE_CONTENT);
                lv_obj_update_layout(_distanceContainer);
                lv_obj_center(_distanceLabel);

                if(lv_obj_has_flag(_directionLabel, LV_OBJ_FLAG_HIDDEN))
                {
                    lv_obj_clear_flag(_directionLabel, LV_OBJ_FLAG_HIDDEN);
                    // Setel ulang posisi gulir teks untuk mulai berjalan dari awal
                    // Setel ulang teks untuk memulai efek gulir dari awal
                    const String currentText = String(lv_label_get_text(_directionLabel));
                    lv_label_set_text(_directionLabel, "");  // Hapus teks
                    lv_label_set_text(_directionLabel, currentText.c_str());  // Setel ulang teks

                    // Paksa pembaruan dan gambar ulang
                    lv_obj_invalidate(_directionLabel);
                }
            }
        }

        // Perbarui data baru
        _navData = navData;

        // Perbarui tampilan label pesan default
        if (_defaultMessageLabel) {
            if (willBeActive) {
                // Jika navigasi aktif, sembunyikan pesan default
                lv_obj_add_flag(_defaultMessageLabel, LV_OBJ_FLAG_HIDDEN);
                Serial.println("updateNavigation: Hiding default message");
            } else {
                if(lv_obj_has_flag(_defaultMessageLabel, LV_OBJ_FLAG_HIDDEN))
                {
                    lv_obj_clear_flag(_defaultMessageLabel, LV_OBJ_FLAG_HIDDEN);
                    // Reset posisi scroll text supaya mulai dari awal
                    // Set ulang text supaya efek scroll mulai dari awal
                    const String currentText = String(lv_label_get_text(_defaultMessageLabel));
                    lv_label_set_text(_defaultMessageLabel, "");  // Hapus text
                    lv_label_set_text(_defaultMessageLabel, currentText.c_str());  // Set ulang text

                    // Paksa update dan redraw
                    lv_obj_invalidate(_defaultMessageLabel);
                }
            }
        }

        // Perbarui status ikon - Selalu gambar ulang untuk menghindari hilangnya data ikon yang terfragmentasi dari BLE
        _hasValidIcon = navData.hasIcon;
        if (_hasValidIcon) {
            // Salin data ikon ke buffer alih-alih hanya menyimpan penunjuk
            memcpy(_iconBuffer, navData.icon, ICON_DATA_SIZE);
            _iconData = _iconBuffer;
            _iconCRC = navData.iconCRC;
            // Gambar ikon secara langsung
            drawNavIconDirectly();
        } else if (!_hasValidIcon && _navIcon != nullptr) {
            lv_obj_del(_navIcon);
            _navIcon = nullptr;
            createContent();
        }

        // Perbarui label teks
        updateLabels();
    }

    // Buat layar navigasi
    void create() {
        // Inisialisasi font aplikasi
        AppFonts::init();

        // Hapus layar lama jika ada
        if (_screen) {
            // Hapus semua komponen anak sebelum menghapus layar utama
            lv_obj_clean(_screen);

            // Kemudian hapus layar
            lv_obj_del(_screen);
            _screen = nullptr;

            // Setel pointer lain ke NULL
            _navIcon = nullptr;
            _directionLabel = nullptr;
            _distanceLabel = nullptr;
            _titleLabel = nullptr;
            _timeLabel = nullptr;
            _speedLabel = nullptr;
            _durationLabel = nullptr;
            _bgImage = nullptr;
            _defaultMessageLabel = nullptr;
            _defaultMessageContainer = nullptr;
            _inactiveTimeLabel = nullptr;
            _inactiveDateLabel = nullptr;
            _inactiveBleLabel = nullptr;
        }

        // Buat layar baru
        _screen = lv_obj_create(NULL);

        // Periksa apakah layar berhasil dibuat
        if (!_screen) {
            Serial.println("ERROR: Failed to create navigation screen!");
            return;
        }

        // Buat gambar latar dari bg.c
        _bgImage = lv_img_create(_screen);
        lv_img_set_src(_bgImage, &bg_img);
        lv_obj_align(_bgImage, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_img_opa(_bgImage, 255, LV_PART_MAIN); // Tampilkan 100% jelas

        // Setel warna latar belakang untuk layar
        lv_obj_set_style_bg_color(_screen, lv_color_hex(NavColors::BackgroundHex), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(_screen, 0, LV_PART_MAIN); // 100% transparan untuk menampilkan gambar latar dengan jelas

        // Buat komponen UI
        createHeader();
        createContent();
        createInactiveDashboard();

        updateUIBasedOnNavigationState();

        // Perbarui label dengan data nyata dengan segera
        updateLabels();

        Serial.println("NavigationScreenLVGL: Screen created successfully");
    }

    // Tampilkan layar
    void display() {
        if (_screen) {
            // Sinkronkan status sembunyikan/tampilkan rute/komponen siaga sebelum menggambar
            updateUIBasedOnNavigationState();

            // Perbarui data sebelum menampilkan
            updateLabels();

            // Pastikan gambar latar ditampilkan dengan benar dan jelas
            if (_bgImage) {
                lv_obj_move_background(_bgImage); // Pastikan gambar latar ada di bagian bawah
            }

            // Pindahkan pesan default ke atas gambar latar jika sedang ditampilkan
            if (_defaultMessageContainer && !lv_obj_has_flag(_defaultMessageContainer, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_move_foreground(_defaultMessageContainer);
                lv_obj_move_foreground(_defaultMessageLabel);
            }
            if (_inactiveTimeLabel && !lv_obj_has_flag(_inactiveTimeLabel, LV_OBJ_FLAG_HIDDEN)) {
                lv_obj_move_foreground(_inactiveTimeLabel);
            }

            // Tentukan status aktif navigasi
            bool isActive = _navData.active && _navData.isNavigation;

            // Hanya tampilkan komponen UI saat navigasi aktif
            if (isActive) {
                // Pindahkan semua elemen UI ke atas gambar latar
                if (_navIcon) lv_obj_move_foreground(_navIcon);
                if (_directionLabel && !lv_obj_has_flag(_directionLabel, LV_OBJ_FLAG_HIDDEN)) 
                    lv_obj_move_foreground(_directionLabel);
                if (_titleLabel) lv_obj_move_foreground(_titleLabel);
                if (_timeLabel) lv_obj_move_foreground(_timeLabel);
                if (_speedLabel && !lv_obj_has_flag(_speedLabel, LV_OBJ_FLAG_HIDDEN)) 
                    lv_obj_move_foreground(_speedLabel);
                if (_durationLabel) lv_obj_move_foreground(_durationLabel);

                // Pastikan wadah jarak dan label ditampilkan di bagian atas
                if (_distanceContainer && !lv_obj_has_flag(_distanceContainer, LV_OBJ_FLAG_HIDDEN)) {
                    // Hapus bilah gulir
                    lv_obj_clear_flag(_distanceContainer, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_clear_flag(_distanceLabel, LV_OBJ_FLAG_SCROLLABLE);

                    // Pastikan ukuran wadah sesuai
                    lv_obj_set_width(_distanceContainer, LV_SIZE_CONTENT);
                    lv_obj_update_layout(_distanceContainer);
                    lv_obj_center(_distanceLabel);

                    // Bawa ke atas
                    lv_obj_move_foreground(_distanceContainer);
                    lv_obj_move_foreground(_distanceLabel);

                    Serial.printf("Display: Distance container size: w=%d, h=%d\n", 
                                lv_obj_get_width(_distanceContainer), 
                                lv_obj_get_height(_distanceContainer));
                }
            }

            // Beralih ke layar navigasi
            lv_scr_load(_screen);

            // Panggil pembaruan LVGL untuk menggambar dengan segera
            lv_refr_now(NULL);
        } else {
            Serial.println("ERROR: NavigationScreenLVGL - _screen is NULL, cannot display");
        }
    }

public:
    // Perbarui antarmuka berdasarkan status aktif/tidak aktif navigasi
    void updateUIBasedOnNavigationState() {
        bool isActive = _navData.active && _navData.isNavigation;

        if(isActive)
        {
            // Sembunyikan pesan default dan dasbor terlebih dahulu
            if (_defaultMessageContainer) lv_obj_add_flag(_defaultMessageContainer, LV_OBJ_FLAG_HIDDEN);
            if (_inactiveTimeLabel) lv_obj_add_flag(_inactiveTimeLabel, LV_OBJ_FLAG_HIDDEN);

            // Tampilkan komponen UI
            lv_obj_clear_flag(_directionLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_speedLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_titleLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_timeLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_durationLabel, LV_OBJ_FLAG_HIDDEN);
            if(_navIcon)
            {
                lv_obj_clear_flag(_navIcon, LV_OBJ_FLAG_HIDDEN);
            }

            // Tampilkan wadah jarak dan perbarui ukuran
            lv_obj_clear_flag(_distanceContainer, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_distanceLabel, LV_OBJ_FLAG_HIDDEN);

            // Paksa perbarui ukuran wadah berdasarkan konten
            lv_obj_set_width(_distanceContainer, LV_SIZE_CONTENT);
            lv_obj_update_layout(_distanceContainer);
            lv_obj_center(_distanceLabel);

            Serial.println("Navigation active: Showing all UI elements");
        }
        else
        {
            // Tampilkan pesan default dan dasbor
            if (_inactiveTimeLabel) lv_obj_clear_flag(_inactiveTimeLabel, LV_OBJ_FLAG_HIDDEN);
            if (_defaultMessageContainer) {
                lv_obj_clear_flag(_defaultMessageContainer, LV_OBJ_FLAG_HIDDEN);
                const String currentText = String(lv_label_get_text(_defaultMessageLabel));
                lv_label_set_text(_defaultMessageLabel, "");  // Hapus text
                lv_label_set_text(_defaultMessageLabel, currentText.c_str());  // Set ulang text

                // Paksa update dan redraw
                lv_obj_invalidate(_defaultMessageLabel);
            }


            // Sembunyikan semua komponen UI lainnya
            lv_obj_add_flag(_directionLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_speedLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_distanceContainer, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_distanceLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_titleLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_timeLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_durationLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_durationLabel, LV_OBJ_FLAG_HIDDEN);
            if(_navIcon)
            {
                lv_obj_add_flag(_navIcon, LV_OBJ_FLAG_HIDDEN);
            }

            Serial.println("Navigation inactive: Only showing background and default message");
        }

        // Catat status
        Serial.printf("Updated UI based on navigation state: isActive=%d\n", isActive);
    }

    private:

    // Buat header (bilah status)
    void createHeader() {
        // Buat label waktu di tepi kiri
        _timeLabel = AppFonts::createText(
            _screen, 
            "", 
            AppFonts::getNormalFont(), 
            lv_color_hex(0xFFFFFF),  // Putih
            LV_ALIGN_TOP_LEFT,
            5, 5  // Posisi margin kiri atas
        );

        // Buat label durasi di margin kanan
        _durationLabel = AppFonts::createText(
            _screen, 
            "", 
            AppFonts::getNormalFont(), 
            lv_color_hex(0xFFFFFF),  // Putih
            LV_ALIGN_TOP_RIGHT,
            -5, 5  // Posisi tepi kanan atas
        );

        // Setel warna teks secara langsung, tanpa latar belakang
        lv_obj_set_style_text_color(_timeLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // Putih
        lv_obj_set_style_text_opa(_timeLabel, 255, LV_PART_MAIN); // Opasitas maksimum
        lv_obj_set_style_text_color(_durationLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // Putih
        lv_obj_set_style_text_opa(_durationLabel, 255, LV_PART_MAIN); // Opasitas maksimum
    }

    // Buat konten (petunjuk arah)
    void createContent() {
        // Buat ikon navigasi di tepi kiri
        if (_hasValidIcon && _iconData != nullptr) {
            // Gambar ikon secara langsung menggunakan API bitmap LVGL
            drawNavIconDirectly();
        }

        // Buat label judul yang menampilkan instruksi berikutnya (di bawah Y:142)
        _titleLabel = lv_label_create(_screen);
        lv_obj_align(_titleLabel, LV_ALIGN_TOP_LEFT, 5, 142);
        lv_obj_set_width(_titleLabel, 230); // Batas lebar
        lv_obj_set_style_text_font(_titleLabel, AppFonts::getNormalFont(), LV_PART_MAIN); // Font reguler lebih kecil (24px)
        lv_obj_set_style_text_color(_titleLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // Putih agar sangat jelas
        lv_obj_set_style_text_opa(_titleLabel, 255, LV_PART_MAIN);

        lv_label_set_long_mode(_titleLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_anim_speed(_titleLabel, 30, LV_PART_MAIN); // Kecepatan gulir sedikit lebih lambat
        lv_obj_set_style_anim_time(_titleLabel, 5000, LV_PART_MAIN);

        lv_label_set_text(_titleLabel, "");

        // Buat label panduan utama (sederhana, Y:110)
        _directionLabel = lv_label_create(_screen);
        lv_obj_align(_directionLabel, LV_ALIGN_TOP_LEFT, 5, 110);
        lv_obj_set_width(_directionLabel, 230); // Batas lebar
        lv_obj_set_style_text_font(_directionLabel, AppFonts::getSemiboldFont(), LV_PART_MAIN);
        lv_obj_set_style_text_color(_directionLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_opa(_directionLabel, 255, LV_PART_MAIN);

        // Setel mode gulir dan parameter animasi hanya sekali
        lv_label_set_long_mode(_directionLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_anim_speed(_directionLabel, 40, LV_PART_MAIN);
        lv_obj_set_style_anim_time(_directionLabel, 5000, LV_PART_MAIN);

        lv_label_set_text(_directionLabel, "");

        // Buat label kecepatan di sebelah kanan ikon (Y:50, X:-10)
        _speedLabel = lv_label_create(_screen);
        lv_obj_align(_speedLabel, LV_ALIGN_TOP_RIGHT, -10, 50);
        lv_obj_set_style_text_font(_speedLabel, AppFonts::getSemiboldFont(), LV_PART_MAIN);
        lv_obj_set_style_text_color(_speedLabel, lv_color_hex(0x00FF00), LV_PART_MAIN); // Hijau sporty menonjol
        lv_obj_set_style_text_align(_speedLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN); // Pusatkan 2 baris teks

        // Buat wadah untuk jarak dengan latar kuning dan sudut membulat (Y: 178)
        _distanceContainer = lv_obj_create(_screen);

        // Atur ukuran sesuai konten, bukan ukuran tetap
        lv_obj_set_size(_distanceContainer, LV_SIZE_CONTENT, 35);
        lv_obj_align(_distanceContainer, LV_ALIGN_TOP_MID, 0, 178);

        // Atur gaya untuk wadah dengan latar kuning dan sudut membulat
        lv_obj_set_style_bg_color(_distanceContainer, lv_color_hex(0xFFDF00), LV_PART_MAIN); // Kuning
        lv_obj_set_style_bg_opa(_distanceContainer, 255, LV_PART_MAIN); // Opasitas maksimum
        lv_obj_set_style_radius(_distanceContainer, 10, LV_PART_MAIN); // Sudut membulat (dikurangi)
        lv_obj_set_style_pad_all(_distanceContainer, 5, LV_PART_MAIN);  // Padding merata
        lv_obj_set_style_pad_left(_distanceContainer, 15, LV_PART_MAIN); // Tambahkan padding kiri
        lv_obj_set_style_pad_right(_distanceContainer, 15, LV_PART_MAIN); // Tambahkan padding kanan
        lv_obj_set_style_border_width(_distanceContainer, 0, LV_PART_MAIN); // Tidak ada batas

        // Hapus bilah gulir
        lv_obj_clear_flag(_distanceContainer, LV_OBJ_FLAG_SCROLLABLE);

        // Buat label jarak di dalam wadah
        _distanceLabel = lv_label_create(_distanceContainer);
        lv_obj_set_size(_distanceLabel, LV_SIZE_CONTENT, LV_SIZE_CONTENT); // Otomatis sesuaikan ukuran
        lv_obj_set_style_text_font(_distanceLabel, AppFonts::getSemiboldFont(), LV_PART_MAIN); // Font semi-bold supaya teks tidak terpotong
        lv_obj_set_style_text_color(_distanceLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // Warna teks putih
        lv_obj_set_style_text_opa(_distanceLabel, 255, LV_PART_MAIN); // Opasitas maksimum

        // Hilangkan scrollbar untuk label
        lv_obj_clear_flag(_distanceLabel, LV_OBJ_FLAG_SCROLLABLE);

        // Pusatkan di dalam container
        lv_obj_center(_distanceLabel);

        // Contoh teks untuk memeriksa ukuran - pastikan wadah cukup lebar untuk teks apa pun
        lv_label_set_text(_distanceLabel, "0 km");

        // Perbarui tata letak agar wadah berukuran sesuai dengan konten
        lv_obj_update_layout(_distanceContainer);

        // Catat ukuran awal
        Serial.printf("Initial distance container size: w=%d, h=%d\n", 
                    lv_obj_get_width(_distanceContainer), 
                    lv_obj_get_height(_distanceContainer));
    }

    // Buat Dasbor yang ditampilkan saat tidak dalam mode navigasi aktif (hanya tampilkan Jam)
    void createInactiveDashboard() {
        // 1. Jam (Waktu) - Besar di atas (digeser ke bawah sedikit lebih ke tengah)
        _inactiveTimeLabel = lv_label_create(_screen);
        lv_obj_align(_inactiveTimeLabel, LV_ALIGN_TOP_MID, 0, 65);
        lv_obj_set_style_text_font(_inactiveTimeLabel, AppFonts::getNumberBoldFont(), LV_PART_MAIN); // Font angka besar 48px
        lv_obj_set_style_text_color(_inactiveTimeLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_label_set_text(_inactiveTimeLabel, "00:00");

        // 2. Buat wadah kuning dengan sudut membulat untuk pesan default agar kontrasnya bagus di latar belakang abu-abu
        _defaultMessageContainer = lv_obj_create(_screen);
        lv_obj_set_size(_defaultMessageContainer, 230, 35);
        lv_obj_align(_defaultMessageContainer, LV_ALIGN_TOP_MID, 0, 145);
        lv_obj_set_style_bg_color(_defaultMessageContainer, lv_color_hex(0xFFDF00), LV_PART_MAIN); // Kuning
        lv_obj_set_style_bg_opa(_defaultMessageContainer, 255, LV_PART_MAIN);
        lv_obj_set_style_radius(_defaultMessageContainer, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_all(_defaultMessageContainer, 5, LV_PART_MAIN);
        lv_obj_set_style_pad_left(_defaultMessageContainer, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_right(_defaultMessageContainer, 10, LV_PART_MAIN);
        lv_obj_set_style_border_width(_defaultMessageContainer, 0, LV_PART_MAIN);
        lv_obj_clear_flag(_defaultMessageContainer, LV_OBJ_FLAG_SCROLLABLE);

        // 3. Pesan default ada di dalam wadah kuning
        _defaultMessageLabel = lv_label_create(_defaultMessageContainer);
        lv_obj_set_width(_defaultMessageLabel, 210); // Sama dengan lebar wadah dikurangi padding
        lv_obj_set_style_text_font(_defaultMessageLabel, AppFonts::getNormalFont(), LV_PART_MAIN);
        lv_obj_set_style_text_color(_defaultMessageLabel, lv_color_hex(0x000000), LV_PART_MAIN); // Teks hitam untuk kontras terbaik
        lv_obj_set_style_text_opa(_defaultMessageLabel, 255, LV_PART_MAIN);

        lv_label_set_long_mode(_defaultMessageLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_anim_speed(_defaultMessageLabel, 40, LV_PART_MAIN);
        lv_obj_set_style_anim_time(_defaultMessageLabel, 5000, LV_PART_MAIN);
        lv_label_set_text(_defaultMessageLabel, "Start navigation on Google maps");

        // Status awal
        if (_navData.active && _navData.isNavigation) {
            lv_obj_add_flag(_inactiveTimeLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_defaultMessageContainer, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(_inactiveTimeLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_defaultMessageContainer, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Perbarui konten label
    void updateLabels() {
        // Perbarui panduan utama (Gunakan arah dari Chronos sebagai instruksi belokan saat ini)
        if (_directionLabel) {
            if (_navData.directions.length() > 0) {
                const char* currentText = lv_label_get_text(_directionLabel);
                if (strcmp(currentText, _navData.directions.c_str()) != 0) {
                    lv_label_set_text(_directionLabel, _navData.directions.c_str());
                    Serial.println("Updated direction text (from directions): " + _navData.directions);
                }
            } 
        }

        // Perbarui panduan berikutnya (Gunakan judul dari Chronos sebagai sub-panduan di bawah)
        if (_titleLabel) {
            if (_navData.title.length() > 0) {
                const char* currentText = lv_label_get_text(_titleLabel);
                if (strcmp(currentText, _navData.title.c_str()) != 0) {
                    lv_label_set_text(_titleLabel, _navData.title.c_str());
                    Serial.println("Updated next-turn title text (from title): " + _navData.title);
                }
            }
        }

        // Perbarui waktu saat ini (sudut kiri atas)
        String currentTime;
        int hour = ChronosManager::getInstance().getChronos().getHour();
        int min = ChronosManager::getInstance().getChronos().getMinute();
        // Format waktu saat ini "4:04" (tanpa AM/PM)
        currentTime = String(hour) + ":" + (min < 10 ? "0" : "") + String(min);
        lv_label_set_text(_timeLabel, currentTime.c_str());

        // Perbarui jarak di dalam wadah
        String displayDistance = _navData.distance;
        displayDistance.trim();

        // Ekstrak angka dari displayDistance untuk memeriksa apakah itu 0 atau tidak
        float distVal = -1.0;
        if (displayDistance.length() > 0) {
            String numStr = "";
            for (int i = 0; i < displayDistance.length(); i++) {
                char c = displayDistance.charAt(i);
                if (isDigit(c) || c == '.') {
                    numStr += c;
                }
            }
            if (numStr.length() > 0) {
                distVal = numStr.toFloat();
            }
        }

        // Jika jarak kosong, 0, atau berisi 0.0, coba ekstrak jarak dari judul
        if (displayDistance.length() == 0 || distVal == 0.0) {
            String title = _navData.title;

            // Pindai seluruh string judul untuk mencari format "[Angka] m" atau "[Angka] km"
            int numberStart = -1;
            int numberEnd = -1;

            for (int i = 0; i < title.length(); i++) {
                if (isDigit(title.charAt(i))) {
                    if (numberStart == -1) {
                        numberStart = i;
                    }
                    numberEnd = i;
                } else {
                    if (numberStart != -1) {
                        // Menemukan angka, periksa apakah karakter berikutnya (sudah di-trim) adalah "m" atau "km"
                        String suffix = title.substring(i);
                        suffix.trim();

                        if (suffix.startsWith("m") || suffix.startsWith("M")) {
                            // Pastikan kata berikutnya bukan kata lain (misalnya "meter" oke, "masuk" atau "jalan" tidak)
                            if (suffix.length() == 1 || !isAlpha(suffix.charAt(1))) {
                                displayDistance = title.substring(numberStart, numberEnd + 1) + " m";
                                break;
                            }
                        } else if (suffix.startsWith("km") || suffix.startsWith("KM") || suffix.startsWith("Km")) {
                            if (suffix.length() == 2 || !isAlpha(suffix.charAt(2))) {
                                displayDistance = title.substring(numberStart, numberEnd + 1) + " km";
                                break;
                            }
                        }

                        // Setel ulang pencarian nomor baru
                        numberStart = -1;
                        numberEnd = -1;
                    }
                }
            }
        }

        if (displayDistance.length() > 0) {
            lv_label_set_text(_distanceLabel, displayDistance.c_str());
        } else {
            lv_label_set_text(_distanceLabel, "0 m");
        }

        // Setelah memperbarui konten, tata letak wadah harus diperbarui untuk memastikannya ukurannya sesuai
        if (_distanceContainer) {
            // Pastikan ukuran container di-update sesuai konten
            lv_obj_update_layout(_distanceContainer);

            // Pastikan label terpusat di wadah
            lv_obj_center(_distanceLabel);
        }

        // Perbarui kecepatan (di sisi kanan ikon, ditumpuk 2 baris)
        if (_speedLabel) {
            String speedText = _navData.speed;
            speedText.trim();

            // Ekstrak nomor kecepatan
            String numStr = "";
            for (int i = 0; i < speedText.length(); i++) {
                char c = speedText.charAt(i);
                if (isDigit(c) || c == '.') {
                    numStr += c;
                }
            }

            if (numStr.length() > 0) {
                lv_label_set_text(_speedLabel, numStr.c_str());
            } else {
                lv_label_set_text(_speedLabel, "0");
            }
        }
        // Perbarui waktu perjalanan (sudut kanan atas)
        lv_label_set_text(_durationLabel, _navData.duration.c_str());

        // Update Inactive Dashboard (Jam)
        if (_inactiveTimeLabel && !lv_obj_has_flag(_inactiveTimeLabel, LV_OBJ_FLAG_HIDDEN)) {
            char timeStr[10];
            sprintf(timeStr, "%02d:%02d", hour, min);
            lv_label_set_text(_inactiveTimeLabel, timeStr);
        }

        // Pastikan komponen diperbarui dengan segera
        if (_bgImage) {
            lv_obj_invalidate(_bgImage); // Pastikan background digambar ulang lebih dulu
        }
        lv_obj_invalidate(_screen);
    }
};

#endif // NAVIGATION_SCREEN_LVGL_H
