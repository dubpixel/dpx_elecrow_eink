#include "epd_panel.h"
#include "epd_ll.h"

void CrowPanel579::begin(epd_rotation_t rotate)
{
  epd_ll_gpio_init();
  epd_ll_init();
  epd_paint_new(_framebuffer, rotate, EPD_WHITE);
  epd_paint_clear(EPD_WHITE);
  epd_ll_clear_ram();
  epd_ll_refresh_full();
  _partial_count = 0;
  _last_was_partial = false;
}

void CrowPanel579::fullRefresh()
{
  epd_ll_write_frame(_framebuffer);
  epd_ll_refresh_full();
  _partial_count = 0;
  _last_was_partial = false;
}

void CrowPanel579::partialRefresh()
{
  if (!_last_was_partial)
  {
    // First partial after a full (or after init) -- safe, no mode-mixing
    // concern since the controller was just fully refreshed.
  }

  epd_ll_write_frame(_framebuffer);

  if (_partial_count >= EPD_PANEL_PARTIAL_REFRESH_LIMIT)
  {
    // Ghosting mitigation the vendor examples skip entirely: force a full
    // refresh periodically instead of partial-refreshing forever.
    Serial.println(F("[epd] partial refresh limit reached, forcing full refresh to clear ghosting"));
    epd_ll_refresh_full();
    _partial_count = 0;
  }
  else
  {
    epd_ll_refresh_partial();
    _partial_count++;
  }
  _last_was_partial = true;
}

void CrowPanel579::sleep()
{
  epd_ll_deep_sleep();
}
