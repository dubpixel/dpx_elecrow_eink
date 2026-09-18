// ================================================================================
// FIRMWARE - dpx_elecrow_eink demo / template entry point
// ================================================================================
// Minimal "does the panel work" sketch for the ELECROW CrowPanel 5.79"
// E-Paper (ESP32-S3-WROOM-1-N8R8). Draws a static layout, does one full
// refresh, one partial refresh, then sleeps -- exercising every safety path
// in the CrowPanel579 wrapper (lib/crowpanel_579) without needing WiFi or
// any external service to prove the driver out.
// ================================================================================
// PROJECT: dpx_elecrow_eink
// ================================================================================
//
// File: main.cpp
// Purpose: Bring-up / template sketch for the CrowPanel579 driver.
// Dependencies: CrowPanel579 (lib/crowpanel_579)
//
// CHANGE LOG:
// 2026-09-15: Initial template, replaces vendor demo as the project starting
//             point.
//
// ================================================================================

#include <Arduino.h>
#include "epd_panel.h"

CrowPanel579 panel;

void setup()
{
  Serial.begin(115200);
  delay(200);

  panel.begin(EPD_ROTATE_0);

  panel.clear(EPD_WHITE);
  panel.drawRect(0, 0, EPD_VISIBLE_W - 1, EPD_VISIBLE_H - 1, EPD_BLACK, false);
  // Deliberately NOT centered on x=396 (the visible width's true midpoint)
  // -- that's exactly where the hidden two-controller gap sits. Anchor
  // layouts left/right of it, not straddling it.
  panel.drawString(24, 24, "dpx_elecrow_eink", 24, EPD_BLACK);
  panel.drawString(24, 60, "CrowPanel 5.79in template - fullRefresh()", 16, EPD_BLACK);
  panel.fullRefresh();

  delay(2000);

  panel.drawString(24, 90, "partialRefresh() sample", 16, EPD_BLACK);
  panel.partialRefresh();

  // Always sleep the panel between drawing sessions -- this is what gives
  // this display its near-zero idle power draw. See
  // firmware/mfg_examples/AMAZON_REVIEW_GOTCHAS.md for the field reports
  // this template's safety rails are built from.
  panel.sleep();

  Serial.println(F("[main] draw session complete, panel asleep"));
}

void loop()
{
  // Intentionally empty for the bring-up template. A real project should
  // wake the ESP32 on a timer (esp_sleep_enable_timer_wakeup +
  // esp_deep_sleep_start) rather than spinning here, to get the battery
  // life reviewers report this panel is capable of.
}
