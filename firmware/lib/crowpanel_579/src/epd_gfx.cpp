#include "epd_gfx.h"
#include "epd_ll.h"
#include "epd_font.h"

epd_paint_t epd_paint;

static void epd_warn_once(const __FlashStringHelper *msg)
{
  static uint32_t last_warn_ms = 0;
  uint32_t now = millis();
  // Rate-limit: a bad layout loop calling this every frame shouldn't flood
  // the serial console into uselessness.
  if (now - last_warn_ms > 2000)
  {
    Serial.println(msg);
    last_warn_ms = now;
  }
}

void epd_paint_new(uint8_t *image, uint16_t width, uint16_t height,
                    epd_rotation_t rotate, uint8_t color)
{
  epd_paint.image = image;
  epd_paint.color = color;
  epd_paint.widthMemory = width;
  epd_paint.heightMemory = height;
  epd_paint.widthByte = (width % 8 == 0) ? (width / 8) : (width / 8 + 1);
  epd_paint.rotate = rotate;
  if (rotate == EPD_ROTATE_0 || rotate == EPD_ROTATE_180)
  {
    epd_paint.width = height;
    epd_paint.height = width;
  }
  else
  {
    epd_paint.width = width;
    epd_paint.height = height;
  }
}

void epd_paint_clear(uint8_t color)
{
  uint32_t bytes = (uint32_t)epd_paint.widthByte * epd_paint.heightMemory;
  memset(epd_paint.image, color, bytes);
}

void epd_set_pixel(int32_t x, int32_t y, uint8_t color)
{
  // Clamp to the *visible* logical canvas first. This is the fix for the
  // "coordinates aren't bounds-checked" bug: the vendor driver trusted
  // callers to pass width-1/height-1 and would silently underflow the byte
  // address (and write out of the framebuffer) if the raw width/height was
  // passed instead. Clamping here means a bad caller gets a pixel drawn in
  // the wrong place, never a corrupted buffer or a panic.
  if (x < 0 || x >= epd_paint.width || y < 0 || y >= epd_paint.height)
  {
    epd_warn_once(F("[epd] WARNING: draw coordinate out of range, clamped"));
    x = constrain(x, 0, epd_paint.width - 1);
    y = constrain(y, 0, epd_paint.height - 1);
  }

  uint16_t rx, ry;
  switch (epd_paint.rotate)
  {
  case EPD_ROTATE_0:
    // Two-controller gap: RAM is 800px wide (400px/half) but only 792px of
    // glass is visible, with a permanent ~8px dead strip at logical
    // x 396-403. Any x at or past that boundary needs the +8 RAM offset so
    // it lands on the slave controller's half instead of inside the gap.
    if (x >= 396)
      x += 8;
    rx = x;
    ry = y;
    break;
  case EPD_ROTATE_90:
    if (y >= 396)
      y += 8;
    rx = epd_paint.widthMemory - y - 1;
    ry = x;
    break;
  case EPD_ROTATE_180:
    if (x >= 396)
      x += 8;
    rx = epd_paint.widthMemory - x - 1;
    ry = epd_paint.heightMemory - y - 1;
    break;
  case EPD_ROTATE_270:
    if (y >= 396)
      y += 8;
    rx = y;
    ry = epd_paint.heightMemory - x - 1;
    break;
  default:
    return;
  }

  uint32_t addr = rx / 8 + (uint32_t)ry * epd_paint.widthByte;
  uint8_t bit_mask = 0x80 >> (rx % 8);
  if (color == EPD_BLACK)
    epd_paint.image[addr] &= ~bit_mask;
  else
    epd_paint.image[addr] |= bit_mask;
}

void epd_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color)
{
  int32_t dx = abs(x1 - x0);
  int32_t dy = -abs(y1 - y0);
  int32_t sx = x0 < x1 ? 1 : -1;
  int32_t sy = y0 < y1 ? 1 : -1;
  int32_t err = dx + dy;

  for (;;)
  {
    epd_set_pixel(x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    int32_t e2 = 2 * err;
    if (e2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

void epd_draw_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color, bool filled)
{
  if (filled)
  {
    int32_t y_lo = min(y0, y1), y_hi = max(y0, y1);
    for (int32_t y = y_lo; y <= y_hi; y++)
      epd_draw_line(x0, y, x1, y, color);
  }
  else
  {
    epd_draw_line(x0, y0, x1, y0, color);
    epd_draw_line(x0, y0, x0, y1, color);
    epd_draw_line(x1, y1, x1, y0, color);
    epd_draw_line(x1, y1, x0, y1, color);
  }
}

void epd_draw_circle(int32_t cx, int32_t cy, int32_t radius, uint8_t color, bool filled)
{
  int32_t x = 0, y = radius;
  int32_t d = 3 - (radius << 1);
  while (x <= y)
  {
    if (filled)
    {
      for (int32_t sy = x; sy <= y; sy++)
      {
        epd_set_pixel(cx + x, cy + sy, color);
        epd_set_pixel(cx - x, cy + sy, color);
        epd_set_pixel(cx - sy, cy + x, color);
        epd_set_pixel(cx - sy, cy - x, color);
        epd_set_pixel(cx - x, cy - sy, color);
        epd_set_pixel(cx + x, cy - sy, color);
        epd_set_pixel(cx + sy, cy - x, color);
        epd_set_pixel(cx + sy, cy + x, color);
      }
    }
    else
    {
      epd_set_pixel(cx + x, cy + y, color);
      epd_set_pixel(cx - x, cy + y, color);
      epd_set_pixel(cx - y, cy + x, color);
      epd_set_pixel(cx - y, cy - x, color);
      epd_set_pixel(cx - x, cy - y, color);
      epd_set_pixel(cx + x, cy - y, color);
      epd_set_pixel(cx + y, cy - x, color);
      epd_set_pixel(cx + y, cy + x, color);
    }
    if (d < 0)
      d += 4 * x + 6;
    else
    {
      d += 10 + 4 * (x - y);
      y--;
    }
    x++;
  }
}

void epd_draw_char(int32_t x, int32_t y, char c, uint8_t size, uint8_t color)
{
  const uint8_t *glyph_table;
  uint16_t glyph_bytes;
  if (!epd_font_lookup(size, &glyph_table, &glyph_bytes))
  {
    epd_warn_once(F("[epd] WARNING: unsupported font size (only 12/16/24/48 exist), char dropped"));
    return;
  }

  uint8_t chr_index = (uint8_t)c - ' ';
  const uint8_t *bits = glyph_table + (uint32_t)chr_index * glyph_bytes;
  int32_t x0 = x, y0 = y;
  uint8_t col_height = size; // one glyph column is `size` px tall

  for (uint16_t i = 0; i < glyph_bytes; i++)
  {
    uint8_t byte = bits[i];
    for (uint8_t b = 0; b < 8; b++)
    {
      epd_set_pixel(x, y, (byte & 0x01) ? color : !color);
      byte >>= 1;
      y++;
    }
    x++;
    if ((x - x0) == size / 2)
    {
      x = x0;
      y0 += 8;
    }
    y = y0;
  }
  (void)col_height;
}

void epd_draw_string(int32_t x, int32_t y, const char *s, uint8_t size, uint8_t color)
{
  while (*s)
  {
    epd_draw_char(x, y, *s, size, color);
    s++;
    x += size / 2;
  }
}

void epd_draw_bitmap(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *bitmap, uint8_t color)
{
  int32_t x0 = x;
  int32_t row_bytes = (w + 7) / 8;
  uint32_t idx = 0;
  for (int32_t row = 0; row < h; row++)
  {
    for (int32_t b = 0; b < row_bytes; b++)
    {
      uint8_t byte = bitmap[idx++];
      for (uint8_t bit = 0; bit < 8 && (x - x0) < w; bit++)
      {
        epd_set_pixel(x, y, (byte & 0x80) ? !color : color);
        x++;
        byte <<= 1;
      }
    }
    x = x0;
    y++;
  }
}
