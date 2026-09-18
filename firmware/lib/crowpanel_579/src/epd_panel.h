// ================================================================================
// HEADER - CrowPanel 5.79" high-level panel API
// ================================================================================
// This is the layer a project's main.cpp should actually call. It owns the
// framebuffer, enforces the two refresh-safety rules that Amazon reviewers
// for this exact panel (ASIN B0FX4PDW6M) found the hard way:
//
//   - Don't mix full/partial/fast update modes within a session without an
//     intervening full refresh -- doing so leaves the panel blank even
//     though the busy handshake reports success. This API tracks the last
//     mode used and forces a full refresh on a mode switch automatically,
//     instead of leaving that foot-gun for every caller to remember.
//   - Full updates take ~1.9s and wear the panel per use -- partial
//     refreshes accumulate ghosting over many cycles on real e-paper
//     hardware. This API forces a full refresh every
//     EPD_PANEL_PARTIAL_REFRESH_LIMIT partial refreshes as ghosting
//     mitigation, which the vendor examples don't do at all.
// ================================================================================
// PROJECT: dpx_elecrow_eink
// ================================================================================
//
// File: epd_panel.h
// Purpose: Session-safe wrapper around epd_ll (registers) + epd_gfx
//          (framebuffer) -- init, refresh-mode guard, sleep.
// Dependencies: epd_ll.h, epd_gfx.h
//
// ================================================================================

#pragma once

#include <Arduino.h>
#include "epd_gfx.h"
#include "epd_ll.h" // EPD_WHITE/EPD_BLACK, EPD_RAM_W/EPD_RAM_H

// Force a full refresh after this many consecutive partial refreshes, even
// if the caller never asked for one. Picked conservatively; tune per your
// own ghosting tolerance if you have real hardware to eyeball it against.
#ifndef EPD_PANEL_PARTIAL_REFRESH_LIMIT
#define EPD_PANEL_PARTIAL_REFRESH_LIMIT 10
#endif

class CrowPanel579
{
public:
  // Allocates and owns the framebuffer, brings up GPIO + panel registers.
  // Call once from setup(). `rotate` matches epd_gfx's epd_rotation_t.
  void begin(epd_rotation_t rotate = EPD_ROTATE_0);

  void clear(uint8_t color = EPD_WHITE) { epd_paint_clear(color); }

  void drawPixel(int32_t x, int32_t y, uint8_t color) { epd_set_pixel(x, y, color); }
  void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color) { epd_draw_line(x0, y0, x1, y1, color); }
  void drawRect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color, bool filled = false) { epd_draw_rect(x0, y0, x1, y1, color, filled); }
  void drawCircle(int32_t cx, int32_t cy, int32_t r, uint8_t color, bool filled = false) { epd_draw_circle(cx, cy, r, color, filled); }
  void drawString(int32_t x, int32_t y, const char *s, uint8_t size, uint8_t color) { epd_draw_string(x, y, s, size, color); }
  void drawBitmap(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *bmp, uint8_t color) { epd_draw_bitmap(x, y, w, h, bmp, color); }

  // Pushes the framebuffer and does a full refresh (slow, ~1.9s, no
  // ghosting). Always safe to call regardless of what came before.
  void fullRefresh();

  // Pushes the framebuffer and does a partial refresh (fast, but the panel
  // accumulates ghosting and this API will silently promote to a full
  // refresh every EPD_PANEL_PARTIAL_REFRESH_LIMIT calls).
  void partialRefresh();

  // Cuts the controller into deep sleep. Call this after every drawing
  // session -- the vendor examples do this too, and reviewers confirm the
  // panel itself draws ~0 idle power only when this is respected. Requires
  // epd_ll_init() (a hardware reset) before the next draw.
  void sleep();

private:
  uint8_t _framebuffer[27200]; // EPD_HALF_PANEL_BYTES * 2, see epd_ll.h
  uint8_t _partial_count = 0;
  bool _last_was_partial = false;
};
