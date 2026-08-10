# PRD & Roadmap — ESP32-C3 Smartwatch Companion (Chronos BLE)

## 1. Ringkasan Produk

Perangkat wearable/dashboard berbasis **ESP32-C3 Super Mini** + layar **ST7789 IPS 240x240**, yang berperan sebagai "smartwatch" companion untuk aplikasi **Chronos** (Android) via **BLE**. Saat HP menjalankan navigasi Google Maps, menerima notifikasi, atau menampilkan cuaca, data tersebut dikirim ke ESP32 dan ditampilkan di layar secara real-time.

**Referensi dasar:** project `navigation_c3` (Chronos + LVGL + LovyanGFX) yang sudah kita bedah lengkap — dipakai ulang sebagian besar (BLE stack, pin GPIO, pola video idle), diperluas dengan home dashboard dan notification overlay.

---

## 2. Tujuan & Non-Tujuan

### Tujuan (in-scope)
- Menampilkan **navigasi Google Maps** fullscreen (arah, jarak, ETA, ikon belok) saat aktif
- Menampilkan **notifikasi HP** sebagai popup overlay sementara, di atas layar apa pun
- Menampilkan **cuaca** di home dashboard
- **Home dashboard**: jam + cuaca, tampil default setelah BLE connect / setelah navigasi selesai
- **Video idle**: animasi JPEG saat boot (sebelum BLE connect) dan sebagai screensaver periodik dari home

### Non-Tujuan (out-of-scope, versi ini)
- Kontrol musik / now-playing (di-skip — library patched belum implement protokolnya)
- Kontrol via tombol fisik untuk ganti layar manual (semua transisi otomatis berdasarkan event BLE)
- Fitur kesehatan (steps, heart rate, dll)
- Kontak & SOS

---

## 3. Target Perangkat & Batasan Teknis

| Item | Spesifikasi |
|---|---|
| MCU | ESP32-C3 Super Mini (RISC-V, single-core, **tanpa PSRAM**, ~400KB SRAM) |
| Layar | ST7789 IPS 240x240, SPI |
| Konektivitas | BLE (NimBLE), protokol Nordic UART Service (kompatibel Chronos app) |
| Framework | Arduino + PlatformIO |
| Library render | LVGL + LovyanGFX (driver bawah) |

**Risiko utama:** keterbatasan RAM. Mitigasi: hanya 1 screen LVGL aktif dalam satu waktu, buffer video di-decode per-frame (bukan preload semua), font custom secukupnya (jangan generate semua ukuran).

---

## 4. State Machine (Spesifikasi Perilaku)

```
[BOOT_VIDEO] --(BLE connected)--> [HOME]
[HOME] --(nav aktif)--> [NAVIGATION]
[NAVIGATION] --(nav selesai)--> [HOME]
[HOME] --(diam 15 detik, tanpa nav)--> [IDLE_VIDEO]
[IDLE_VIDEO] --(1 loop video selesai)--> [HOME]
[IDLE_VIDEO] --(nav aktif, kapan saja)--> [NAVIGATION]   // interrupt langsung
[* apa pun] --(BLE disconnect)--> [BOOT_VIDEO]
[* apa pun] --(notifikasi masuk)--> tampilkan NOTIF_OVERLAY (layer, bukan ganti state)
```

**Aturan penting:**
- Transisi ke `NAVIGATION` **selalu prioritas tertinggi** — memotong video kapan pun, tanpa menunggu.
- `NOTIF_OVERLAY` tidak pernah mengubah state dasar; auto-hilang ~3 detik lalu layar di bawahnya terlihat lagi seperti semula.
- `IDLE_VIDEO` hanya terjadi dari `HOME`, bukan dari `NAVIGATION`.

---

## 5. Functional Requirements

### FR-1: Koneksi BLE (Chronos Protocol)
- Perangkat advertise sebagai BLE peripheral dengan nama watch, terlihat oleh app Chronos
- Terima & parse paket sesuai protokol Chronos (prefix `0xAB`/`0xEA`, opcode per fitur)
- Reconnect otomatis / re-advertise setelah disconnect

### FR-2: Navigasi
- Terima data: title, directions, distance, duration, eta, speed, ikon arah (bitmap 48x48 1bpp)
- Convert ikon 1bpp → RGB565 untuk ditampilkan LVGL
- Auto-scroll teks kalau lebih panjang dari lebar layar
- Tampil fullscreen, prioritas tertinggi

### FR-3: Notifikasi
- Terima: app, title, message, icon, waktu
- Render sebagai popup overlay (bukan fullscreen) — desain baru, referensi lama cuma log Serial
- Auto-dismiss setelah durasi tertentu (default 3 detik, bisa disesuaikan)
- Tidak mengganggu/reset state layar di baliknya

### FR-4: Cuaca
- Terima kota cuaca (`CF_WEATHER` city) — **sudah ada di referensi**
- Terima hourly forecast — **stub kosong di referensi, perlu diimplementasikan**: parsing data per jam (suhu, kondisi) sesuai spek protokol Chronos asli
- Tampil sebagai widget kecil di home dashboard (bukan layar penuh)

### FR-5: Home Dashboard
- Tampilkan jam real-time (sinkron dari `CF_TIME`)
- Tampilkan info cuaca ringkas (ikon kondisi + suhu terkini minimal)
- Default screen setelah connect BLE / setelah navigasi berakhir

### FR-6: Video Idle
- Boot: loop video sebelum BLE connect
- Idle: satu putaran video setelah 15 detik tanpa aktivitas di home, lalu balik ke home
- Decode JPEG per-frame dari flash (PROGMEM), bukan preload ke RAM

---

## 6. Komponen Teknis (Reuse vs Baru)

| Komponen | Status | Keterangan |
|---|---|---|
| `LGFX_Config.h` | ✅ Reuse | Pin GPIO ST7789 sudah sesuai |
| `LVGL_Config.h` | ✅ Reuse | Driver + buffer LVGL |
| `ChronosESP32Patched.h/.cpp` | 🔧 Reuse + extend | Tambah parsing hourly forecast |
| `ChronosESP32Adapter.h`, `ChronosTypes.h` | ✅ Reuse | Tambah tipe data cuaca jika perlu |
| `NavigationScreenLVGL.h` | ✅ Reuse (minor tweak) | Sudah cukup matang |
| Pola video JPEG (`full1.h` generator) | ✅ Reuse pola | Video baru perlu di-generate ulang sesuai aset kamu |
| `VietnameseFonts.h` | 🔧 Reuse, disederhanakan | Rename/generalisasi jika tidak butuh charset Vietnam |
| `HomeScreenLVGL.h` | 🆕 Baru | Jam + widget cuaca |
| `NotificationOverlay.h` | 🆕 Baru | Popup LVGL, referensi lama cuma Serial log |
| `AppStateMachine.h` | 🆕 Baru | Pengganti `PlayerMode`, handle 4 state + overlay |
| Parsing hourly forecast | 🆕 Baru | Isi stub kosong di `ChronosESP32Patched.cpp` |

---

## 7. Roadmap Pengembangan

### Fase 0 — Persiapan Project (skeleton)
- Setup PlatformIO project baru (copy `platformio.ini`, pin config dari referensi)
- Port `LGFX_Config.h`, `LVGL_Config.h` — test layar nyala + LVGL render teks dasar
- **Milestone:** layar nyala, tampil "Hello World" via LVGL ✅ checkpoint sebelum lanjut

### Fase 1 — BLE & Data Layer
- Port `ChronosESP32Patched`, `ChronosESP32Adapter`, `ChronosTypes`
- Test: ESP32 muncul di app Chronos, status connect/disconnect ke Serial Monitor
- **Milestone:** BLE connect terdeteksi, data time & battery terkirim/terima normal

### Fase 2 — State Machine Inti
- Bangun `AppStateMachine.h`: 4 state (`BOOT_VIDEO`, `HOME`, `NAVIGATION`, `IDLE_VIDEO`) + transisi
- Sementara pakai layar placeholder polos per state (kotak warna beda) untuk validasi logika transisi dulu, sebelum UI detail
- **Milestone:** transisi antar state sesuai aturan di §4, terverifikasi dari Serial log

### Fase 3 — Layar Navigasi
- Port `NavigationScreenLVGL.h`, sambungkan ke state machine
- Test end-to-end: buka Google Maps di HP → layar navigasi muncul fullscreen dengan data real
- **Milestone:** navigasi real-time berjalan, interrupt dari state lain berfungsi

### Fase 4 — Home Dashboard
- Bangun `HomeScreenLVGL.h`: jam real-time
- Tambah widget cuaca (setelah Fase 5 selesai parsing data cuaca)
- **Milestone:** home tampil jam berjalan akurat

### Fase 5 — Cuaca
- Implementasi parsing hourly forecast di `ChronosESP32Patched.cpp` (isi stub kosong)
- Sambungkan data ke widget cuaca di home dashboard
- **Milestone:** cuaca dari HP tampil benar di layar (kota + suhu minimal)

### Fase 6 — Video Idle
- Siapkan aset video (convert ke frame JPEG PROGMEM, seperti pola `full1.h`)
- Integrasi ke `BOOT_VIDEO` dan `IDLE_VIDEO` state
- **Milestone:** video muter saat boot, dan siklus 15 detik di home berjalan tanpa lag/crash RAM

### Fase 7 — Notifikasi
- Bangun `NotificationOverlay.h` (popup LVGL, layer di atas)
- Sambungkan ke event notifikasi Chronos, auto-dismiss
- **Milestone:** notifikasi HP muncul sebagai popup di atas layar apa pun, hilang otomatis, tidak break state di baliknya

### Fase 8 — Stabilisasi & Optimasi
- Uji stres: BLE disconnect/reconnect berulang, notifikasi beruntun, transisi cepat nav on/off
- Cek memory leak / heap fragmentation (umum di project LVGL+BLE lama-lama jalan)
- Kalibrasi warna, font, posisi elemen (UI polish)
- **Milestone:** perangkat stabil jalan berjam-jam tanpa reboot/crash

---

## 8. Kriteria Sukses (Definition of Done)

- [ ] Layar boot video → home → navigasi → kembali home berjalan otomatis sesuai state machine
- [ ] Notifikasi HP muncul sebagai popup, tidak mengganggu layar di baliknya
- [ ] Cuaca tampil akurat di home dashboard
- [ ] Tidak ada crash/reboot dalam pemakaian normal ≥ 2 jam
- [ ] BLE reconnect otomatis setelah HP keluar jangkauan lalu kembali

---

## 9. Open Questions (untuk didiskusikan saat implementasi)

- Durasi persis auto-dismiss notifikasi (default 3 detik — cukup?)
- Berapa banyak notifikasi ditampilkan kalau datang beruntun (antrian vs cuma yang terbaru)
- Sumber aset video baru (kamu convert sendiri, atau pakai ulang `full1.h` referensi dulu untuk testing)
