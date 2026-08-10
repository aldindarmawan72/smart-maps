#include <Arduino.h>
#include <lvgl.h>
#include "LVGL_Config.h"
#include "ChronosManager.h"
#include "AppStateMachine.h"

// ============================================================
// FASE 2 — State Machine Test
// Tujuan: validasi logika transisi 4-state (PRD paragraf 4) pakai
// 4 layar placeholder warna solid, SEBELUM UI detail per screen
// dibangun (nav = Fase 3, home = Fase 4, video asli = Fase 6).
// ============================================================

LVGL_Display &display = LVGL_Display::getInstance();
ChronosManager &chronos = ChronosManager::getInstance();
AppStateMachine &stateMachine = AppStateMachine::getInstance();

lv_obj_t *placeholderScreens[4];

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

int stateToIndex(AppState s) {
  switch (s) {
    case AppState::BOOT_VIDEO: return 0;
    case AppState::HOME:       return 1;
    case AppState::NAVIGATION: return 2;
    case AppState::IDLE_VIDEO: return 3;
  }
  return 0;
}

void onStateChange(AppState oldState, AppState newState) {
  lv_scr_load(placeholderScreens[stateToIndex(newState)]);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== Fase 2: State Machine Test ===");

  display.init();

  // 4 layar placeholder - warna solid berbeda per state, cuma untuk
  // memvalidasi LOGIKA TRANSISI. UI detail belum di sini.
  createPlaceholderScreen(0, 0x1A1A1A, "BOOT VIDEO", "Menunggu koneksi BLE...\nCari 'Smart Maps Watch' di app Chronos");
  createPlaceholderScreen(1, 0x1E3A5F, "HOME", "BLE connected");
  createPlaceholderScreen(2, 0x1F4D2E, "NAVIGATION", "Navigasi aktif");
  createPlaceholderScreen(3, 0x4A2E5F, "IDLE VIDEO", "Diam 15 detik di HOME");

  lv_scr_load(placeholderScreens[0]); // mulai dari BOOT_VIDEO

  stateMachine.setOnStateChange(onStateChange);

  chronos.init(display.getTft());

  Serial.println("Setup selesai. Amati Serial log untuk urutan transisi state.");
}

void loop() {
  display.update();
  chronos.update();
  stateMachine.update(chronos);
  delay(5);
}
