// ================================================================================
// HEADER - CrowPanel 5.79" E-Paper low-level SSD1683x2 register driver
// ================================================================================
// Adapted from ELECROW's factory example (EPD_Init.cpp/.h,
// 5.79_wifi_http_openweather demo). Register sequences are unchanged from the
// vendor source -- this is the part that talks directly to the two cascaded
// SSD1683 controllers and there was no reason to touch it.
// ================================================================================
// PROJECT: dpx_elecrow_eink
// ================================================================================
//
// File: epd_ll.h
// Purpose: Bit-banged SPI + SSD1683x2 register-level primitives (reset,
//          busy-wait, RAM addressing, refresh triggers, deep sleep).
// Dependencies: epd_pins.h, Arduino core
//
// CHANGE LOG:
// 2026-09-15: Extracted from vendor EPD_Init.cpp/.h, renamed epd_* prefix,
//             pins moved to epd_pins.h. No register-sequence changes.
//
// ================================================================================

#pragma once

#include <Arduino.h>
#include "epd_pins.h"

// The panel is physically 792x272, but the two cascaded SSD1683 controllers
// are addressed as a combined 800x272 RAM (400px per half). The extra 8
// columns land in a permanent hidden gap in the middle of the visible glass
// (logical x 396-403) -- see epd_gfx.h for how drawing coordinates account
// for this. Confirmed independently by a customer teardown/review on the
// Amazon listing (ASIN B0FX4PDW6M) as well as the vendor's own source
// comments.
#define EPD_RAM_W 800
#define EPD_RAM_H 272

#define EPD_WHITE 0xFF
#define EPD_BLACK 0x00

#define EPD_SOURCE_BYTES (400 / 8)
#define EPD_GATE_BITS 272
#define EPD_HALF_PANEL_BYTES (EPD_SOURCE_BYTES * EPD_GATE_BITS)

void epd_ll_gpio_init(void);
void epd_ll_busy_wait(void);
void epd_ll_hw_reset(void);

// Register-level init: picks the temperature-compensated fast-mode LUT.
// Matches vendor EPD_FastMode1Init(). Call once per session before the first
// refresh.
void epd_ll_init(void);

// Refresh triggers. Only one of these should be used per drawing session --
// see epd_gfx's refresh-mode guard for why mixing them silently blanks the
// panel even though the busy handshake reports success.
void epd_ll_refresh_full(void);
void epd_ll_refresh_partial(void);
void epd_ll_refresh_fast(void);

void epd_ll_deep_sleep(void);

// Pushes a full 792x272 (packed as two 396x272 halves) monochrome buffer to
// both controllers' RAM. Does not trigger a refresh -- call one of the
// epd_ll_refresh_* functions afterward.
void epd_ll_write_frame(const uint8_t *image_bw);

// Clears both controllers' RAM directly (bypasses the framebuffer) --
// useful for a fast power-on blank without allocating ImageBW first.
void epd_ll_clear_ram(void);
