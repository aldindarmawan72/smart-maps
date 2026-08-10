# ESP32-C3 Chronos Companion

Smartwatch companion device berbasis **ESP32-C3 Super Mini** + layar **ST7789 240x240**, terhubung ke aplikasi **Chronos** (Android) via BLE untuk menampilkan navigasi Google Maps, notifikasi, dan cuaca.

Lihat dokumen lengkap: [`docs/PRD_dan_Roadmap.md`](docs/PRD_dan_Roadmap.md)

## Hardware
- ESP32-C3 Super Mini
- Layar IPS ST7789 240x240 (SPI)

## Pin Wiring

| ST7789 | ESP32-C3 |
|---|---|
| SCLK | GPIO4 |
| MOSI | GPIO6 |
| DC | GPIO7 |
| RST | GPIO5 |
| CS | tidak dipakai |
| BLK | GPIO7 (via kode) atau 3V3 langsung |

## Build

Project ini pakai **PlatformIO**.

```bash
pio run                # build
pio run -t upload      # flash ke board
pio device monitor      # serial monitor
```

## Status Pengembangan

Lihat roadmap di `docs/PRD_dan_Roadmap.md` untuk fase pengembangan saat ini.

## Struktur Project

```
src/        -> source code utama (.cpp)
include/    -> header (.h)
lib/        -> library lokal (kalau ada)
docs/       -> PRD, roadmap, catatan desain
test/       -> unit test (PlatformIO test runner)
```
