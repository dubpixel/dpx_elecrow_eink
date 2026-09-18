// ================================================================================
// HEADER - CrowPanel bundled ASCII font lookup
// ================================================================================
// The vendor ships one blocky ASCII-only bitmap font in exactly four sizes:
// 12, 16, 24, 48px. That's a hardware/asset limitation, not something worth
// re-deriving here -- epd_font_tables.h is the vendor's EPDfont.h, copied
// verbatim. What this file adds is a lookup that fails loud (via
// epd_gfx.cpp's warning) instead of the vendor behavior of silently drawing
// nothing for any other size.
// ================================================================================
// PROJECT: dpx_elecrow_eink
// ================================================================================
//
// File: epd_font.h
// Purpose: Map a requested font size to its bitmap table + per-glyph byte
//          count, or report "unsupported" instead of silently no-op'ing.
// Dependencies: epd_font_tables.h (vendor bitmap data, unmodified)
//
// ================================================================================

#pragma once

#include <Arduino.h>
#include "epd_font_tables.h"

// Returns true and fills *table/*glyph_bytes if `size` is one of the four
// sizes the bundled font actually has bitmaps for. Returns false otherwise
// -- callers should treat that as "can't render this size", not draw
// garbage.
inline bool epd_font_lookup(uint8_t size, const uint8_t **table, uint16_t *glyph_bytes)
{
  switch (size)
  {
  case 12:
    *table = &ascii_1206[0][0];
    *glyph_bytes = 12;
    return true;
  case 16:
    *table = &ascii_1608[0][0];
    *glyph_bytes = 16;
    return true;
  case 24:
    *table = &ascii_2412[0][0];
    *glyph_bytes = 36;
    return true;
  case 48:
    *table = &ascii_4824[0][0];
    *glyph_bytes = 144;
    return true;
  default:
    return false;
  }
}
