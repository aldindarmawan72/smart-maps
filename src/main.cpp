#include <Arduino.h>
#include <lvgl.h>
#include "LVGL_Config.h"
#include "ChronosManager.h"
#include "AppStateMachine.h"
#include "NavigationScreenLVGL.h"
#include "Config.h"

// ============================================================
// FASE 3 — Layar Navigasi
// Tujuan: NavigationScreenLVGL (di-port persis dari referensi)
// disambungkan ke AppStateMachine. Placeholder cuma tersisa untuk
// BOOT_VIDEO, HOME, IDLE_VIDEO (belum dibangun - Fase 4 & 6).
// ============================================================

LVGL_Display &display = LVGL_Display::getInstance();
ChronosManager &chronos = ChronosManager::getInstance();
AppStateMachine &stateMachine = AppStateMachine::getInstance();
NavigationScreenLVGL navScreen;

// index 0 = BOOT_VIDEO, 1 = HOME, 2 = IDLE_VIDEO (NAVIGATION pakai navScreen, bukan placeholder)
lv_obj_t *placeholderScreens[3];

void createPlaceholderScreen(int index, uint32_t bgColorHex, const char* title, const char* subtitle) {
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(bgColorHex), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(scr, 0, 0);

  lv_obj_t *lbl = lv_label_create(scr);
  lv_label_set_text(lbl, title);
  lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
  lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -10);

  lv_obj_t *sub = lv_label_create(scr);
  lv_label_set_text(sub, subtitle);
  lv_obj_set_style_text_color(sub, lv_color_hex(0xCCCCCC), 0);
  lv_obj_set_width(sub, 200);
  lv_label_set_long_mode(sub, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(sub, LV_ALIGN_CENTER, 0, 20);

  placeholderScreens[index] = scr;
}

void onStateChange(AppState oldState, AppState newState) {
  if (newState == AppState::NAVIGATION) {
    // Layar navigasi asli, bukan placeholder
    navScreen.display();
    return;
  }

  switch (newState) {
    case AppState::BOOT_VIDEO: lv_scr_load(placeholderScreens[0]); break;
    case AppState::HOME:       lv_scr_load(placeholderScreens[1]); break;
    case AppState::IDLE_VIDEO: lv_scr_load(placeholderScreens[2]); break;
    default: break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== Fase 3: Layar Navigasi Test ===");

  display.init();

  // 3 layar placeholder yang masih tersisa (belum dibangun - Fase 4 & 6)
  createPlaceholderScreen(0, 0x1A1A1A, "BOOT VIDEO", "Menunggu koneksi BLE...\nCari 'Smart Maps Watch' di app Chronos");
  createPlaceholderScreen(1, 0x1E3A5F, "HOME", "BLE connected");
  createPlaceholderScreen(2, 0x4A2E5F, "IDLE VIDEO", "Diam 15 detik di HOME");

  lv_scr_load(placeholderScreens[0]); // mulai dari BOOT_VIDEO

  stateMachine.setOnStateChange(onStateChange);

  chronos.init(display.getTft());

  // Buat layar navigasi sekali di awal (widget-widgetnya dibuat sekali,
  // datanya di-update terus tiap loop() selama state NAVIGATION aktif)
  navScreen.create();

  Serial.println("Setup selesai. Amati Serial log untuk urutan transisi state.");
}

void loop() {
  display.update();
  chronos.update();
  stateMachine.update(chronos);

  // Selama di state NAVIGATION, update data navigasi secara periodik
  // (pola sama seperti NavigationManagerLVGL di referensi: interval
  // dari Config::NAV_UPDATE_INTERVAL, lalu updateNavigation() + display())
  if (stateMachine.getState() == AppState::NAVIGATION) {
    static unsigned long lastNavUpdate = 0;
    unsigned long now = millis();
    if (now - lastNavUpdate >= Config::NAV_UPDATE_INTERVAL) {
      lastNavUpdate = now;
      navScreen.updateNavigation(chronos.getNavData());
      navScreen.display();
    }
  }

  delay(5);
}
