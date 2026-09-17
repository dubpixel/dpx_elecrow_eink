// ================================================================================
// HEADER - CrowPanel 5.79" E-Paper framebuffer + drawing primitives
// ================================================================================
// Rewritten from ELECROW's factory EPD.cpp/.h (Paint_* / EPD_Draw* /
// EPD_Show*) to fix two bugs identified from real-world use reported in
// Amazon customer reviews for this panel (ASIN B0FX4PDW6M):
//
//   1. Coordinates were not bounds-checked. Passing a dimension (792/272)
//      instead of the last valid pixel (791/271) as a line/rect endpoint
//      underflowed the byte-address math and wrote out of the framebuffer,
//      which can corrupt heap memory or panic the MCU.
//   2. The panel is physically two cascaded SSD1683 controllers with a
//      permanent hidden ~8px gap at logical x 396-403 (RAM is 800px wide,
//      only 792 is visible glass). Vendor code already offsets for this in
//      Paint_SetPixel; that offset logic is preserved here but is now the
//      only place coordinate translation happens, and it's paired with
//      clamping so a bad caller can't defeat it.
// ================================================================================
// PROJECT: dpx_elecrow_eink
// ================================================================================
//
// File: epd_gfx.h
// Purpose: 1bpp framebuffer + shape/text drawing, safe against out-of-range
//          coordinates.
// Dependencies: epd_ll.h (RAM geometry constants), epd_font.h
//
// CHANGE LOG:
// 2026-09-15: Rewritten from vendor EPD.cpp/.h with bounds clamping added;
//             fixed-mode refresh guard moved to epd_panel.h.
//
// ================================================================================

#pragma once

#include <Arduino.h>

// Visible panel resolution (what your layout math should use everywhere).
// This is NOT the same as the 800x272 RAM geometry in epd_ll.h -- that's an
// internal detail of the two-controller gap, hidden here.
#define EPD_VISIBLE_W 792
#define EPD_VISIBLE_H 272

// Rotation, matches vendor convention: 0/180 swap width and height.
enum epd_rotation_t
{
  EPD_ROTATE_0 = 0,
  EPD_ROTATE_90 = 90,
  EPD_ROTATE_180 = 180,
  EPD_ROTATE_270 = 270,
};

typedef struct
{
  uint8_t *image;
  uint16_t width, height;       // logical (post-rotation) canvas size
  uint16_t widthMemory, heightMemory; // pre-rotation buffer geometry
  uint16_t widthByte;
  uint8_t color;
  epd_rotation_t rotate;
} epd_paint_t;

extern epd_paint_t epd_paint;

// image buffer must be at least EPD_HALF_PANEL_BYTES*2 (27200) bytes.
void epd_paint_new(uint8_t *image, uint16_t width, uint16_t height,
                    epd_rotation_t rotate, uint8_t color);
void epd_paint_clear(uint8_t color);

// Sets one pixel. Coordinates outside [0, width) x [0, height) are clamped
// (with a rate-limited Serial warning) instead of corrupting the buffer.
void epd_set_pixel(int32_t x, int32_t y, uint8_t color);

// Endpoints are inclusive and clamped the same way as epd_set_pixel -- pass
// EPD_VISIBLE_W-1 / EPD_VISIBLE_H-1 as the far edge, not the raw dimension.
void epd_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color);
void epd_draw_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color, bool filled);
void epd_draw_circle(int32_t cx, int32_t cy, int32_t radius, uint8_t color, bool filled);

// Text. Only sizes 12, 16, 24, 48 exist in the bundled ASCII font (vendor
// limitation) -- other sizes are rejected with a Serial warning instead of
// silently drawing nothing.
void epd_draw_char(int32_t x, int32_t y, char c, uint8_t size, uint8_t color);
void epd_draw_string(int32_t x, int32_t y, const char *s, uint8_t size, uint8_t color);

// 1bpp bitmap blit, MSB-first rows, matching image2cpp's
// "Horizontal - 1 bit per pixel" export (the vendor's own converter tool is
// Windows-only; image2cpp is the cross-platform replacement reviewers use).
void epd_draw_bitmap(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *bitmap, uint8_t color);
